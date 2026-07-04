#include "physicsworld.h"

#include <box3d/box3d.h>
#include <glm/gtc/constants.hpp>
#include <unordered_map>

namespace DL {

namespace {

b3Vec3 toB3Vec3(const glm::vec3 &value) {
  return {value.x, value.y, value.z};
}

glm::vec3 toGlm(const b3Vec3 &value) {
  return {value.x, value.y, value.z};
}

glm::vec3 toGlmPos(const b3Pos &value) {
  return {static_cast<float>(value.x), static_cast<float>(value.y),
          static_cast<float>(value.z)};
}

glm::vec4 debugColorToGlm(std::uint32_t color) {
  const float inv255 = 1.0f / 255.0f;
  return {
      static_cast<float>((color >> 16) & 0xFF) * inv255,
      static_cast<float>((color >> 8) & 0xFF) * inv255,
      static_cast<float>(color & 0xFF) * inv255,
      1.0f,
  };
}

PhysicsDebugLine makeDebugLine(const glm::vec3 &start, const glm::vec3 &end,
                               const glm::vec4 &color) {
  return {.start = start, .startColor = color, .end = end, .endColor = color};
}

glm::vec3 transformPoint(const glm::vec3 &position, const glm::quat &rotation,
                         const glm::vec3 &localPoint) {
  return position + rotation * localPoint;
}

void appendCircle(std::vector<PhysicsDebugLine> &lines, const glm::vec3 &position,
                  const glm::quat &rotation, const glm::vec4 &color,
                  float radius, int segments, const glm::vec3 &axisA,
                  const glm::vec3 &axisB, const glm::vec3 &offset = glm::vec3(0.0f)) {
  for (int i = 0; i < segments; ++i) {
    const float angle0 =
        (glm::two_pi<float>() * static_cast<float>(i)) / static_cast<float>(segments);
    const float angle1 = (glm::two_pi<float>() * static_cast<float>(i + 1)) /
                         static_cast<float>(segments);
    const glm::vec3 local0 =
        offset + (std::cos(angle0) * radius) * axisA + (std::sin(angle0) * radius) * axisB;
    const glm::vec3 local1 =
        offset + (std::cos(angle1) * radius) * axisA + (std::sin(angle1) * radius) * axisB;
    lines.push_back(makeDebugLine(transformPoint(position, rotation, local0),
                                  transformPoint(position, rotation, local1), color));
  }
}

void appendBoxLines(std::vector<PhysicsDebugLine> &lines, const glm::vec3 &position,
                    const glm::quat &rotation, const glm::vec3 &halfExtents,
                    const glm::vec4 &color) {
  const glm::vec3 corners[] = {
      {-halfExtents.x, -halfExtents.y, -halfExtents.z},
      {halfExtents.x, -halfExtents.y, -halfExtents.z},
      {halfExtents.x, halfExtents.y, -halfExtents.z},
      {-halfExtents.x, halfExtents.y, -halfExtents.z},
      {-halfExtents.x, -halfExtents.y, halfExtents.z},
      {halfExtents.x, -halfExtents.y, halfExtents.z},
      {halfExtents.x, halfExtents.y, halfExtents.z},
      {-halfExtents.x, halfExtents.y, halfExtents.z},
  };
  constexpr int edges[][2] = {
      {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
      {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
  };
  for (const auto &edge : edges) {
    lines.push_back(makeDebugLine(transformPoint(position, rotation, corners[edge[0]]),
                                  transformPoint(position, rotation, corners[edge[1]]), color));
  }
}

void appendSphereLines(std::vector<PhysicsDebugLine> &lines, const glm::vec3 &position,
                       const glm::quat &rotation, float radius,
                       const glm::vec4 &color) {
  constexpr int kSegments = 24;
  appendCircle(lines, position, rotation, color, radius, kSegments,
               {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
  appendCircle(lines, position, rotation, color, radius, kSegments,
               {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
  appendCircle(lines, position, rotation, color, radius, kSegments,
               {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
}

void appendCapsuleLines(std::vector<PhysicsDebugLine> &lines, const glm::vec3 &position,
                        const glm::quat &rotation, float radius, float height,
                        const glm::vec4 &color) {
  constexpr int kSegments = 18;
  const float halfHeight = height * 0.5f;

  appendCircle(lines, position, rotation, color, radius, kSegments,
               {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, halfHeight, 0.0f});
  appendCircle(lines, position, rotation, color, radius, kSegments,
               {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -halfHeight, 0.0f});

  const glm::vec3 offsets[] = {
      {radius, 0.0f, 0.0f},
      {-radius, 0.0f, 0.0f},
      {0.0f, 0.0f, radius},
      {0.0f, 0.0f, -radius},
  };
  for (const auto &offset : offsets) {
    lines.push_back(makeDebugLine(
        transformPoint(position, rotation, offset + glm::vec3(0.0f, halfHeight, 0.0f)),
        transformPoint(position, rotation, offset + glm::vec3(0.0f, -halfHeight, 0.0f)),
        color));
  }

  for (int i = 0; i < kSegments; ++i) {
    const float angle0 = (glm::pi<float>() * static_cast<float>(i)) /
                         static_cast<float>(kSegments);
    const float angle1 = (glm::pi<float>() * static_cast<float>(i + 1)) /
                         static_cast<float>(kSegments);

    const glm::vec3 yz0(0.0f, std::cos(angle0) * radius, std::sin(angle0) * radius);
    const glm::vec3 yz1(0.0f, std::cos(angle1) * radius, std::sin(angle1) * radius);
    lines.push_back(makeDebugLine(
        transformPoint(position, rotation, yz0 + glm::vec3(0.0f, halfHeight, 0.0f)),
        transformPoint(position, rotation, yz1 + glm::vec3(0.0f, halfHeight, 0.0f)),
        color));
    lines.push_back(makeDebugLine(
        transformPoint(position, rotation, -yz0 + glm::vec3(0.0f, -halfHeight, 0.0f)),
        transformPoint(position, rotation, -yz1 + glm::vec3(0.0f, -halfHeight, 0.0f)),
        color));

    const glm::vec3 xy0(std::cos(angle0) * radius, std::sin(angle0) * radius, 0.0f);
    const glm::vec3 xy1(std::cos(angle1) * radius, std::sin(angle1) * radius, 0.0f);
    lines.push_back(makeDebugLine(
        transformPoint(position, rotation, xy0 + glm::vec3(0.0f, halfHeight, 0.0f)),
        transformPoint(position, rotation, xy1 + glm::vec3(0.0f, halfHeight, 0.0f)),
        color));
    lines.push_back(makeDebugLine(
        transformPoint(position, rotation, -xy0 + glm::vec3(0.0f, -halfHeight, 0.0f)),
        transformPoint(position, rotation, -xy1 + glm::vec3(0.0f, -halfHeight, 0.0f)),
        color));
  }
}

b3Quat toB3Quat(const glm::quat &value) {
  return {{value.x, value.y, value.z}, value.w};
}

glm::quat toGlm(const b3Quat &value) {
  return {value.s, value.v.x, value.v.y, value.v.z};
}

b3BodyType toB3BodyType(PhysicsBodyType type) {
  switch (type) {
  case PhysicsBodyType::Static:
    return b3_staticBody;
  case PhysicsBodyType::Kinematic:
    return b3_kinematicBody;
  case PhysicsBodyType::Dynamic:
    return b3_dynamicBody;
  }
  return b3_staticBody;
}

bool isValidShapeDesc(const PhysicsShapeDesc &shape) {
  switch (shape.type) {
  case PhysicsShapeType::Box:
    return shape.halfExtents.x > 0.0f && shape.halfExtents.y > 0.0f &&
           shape.halfExtents.z > 0.0f;
  case PhysicsShapeType::Sphere:
    return shape.radius > 0.0f;
  case PhysicsShapeType::Capsule:
    return shape.radius > 0.0f && shape.height > 0.0f;
  }
  return false;
}

} // namespace

struct PhysicsWorld::Impl {
  struct BodyEntry {
    std::size_t engineId = 0;
    b3BodyId body = b3_nullBodyId;
    b3ShapeId shape = b3_nullShapeId;
    PhysicsShapeDesc shapeDesc;
    PhysicsShapeType shapeType = PhysicsShapeType::Box;
  };

  b3WorldId world = b3_nullWorldId;
  std::unordered_map<std::size_t, std::unique_ptr<BodyEntry>> bodies;
  std::size_t nextBodyId = 1;
  PhysicsDebugRenderSettings debugSettings;
  bool debugRenderingEnabled = false;

  Impl() {
    b3WorldDef settings = b3DefaultWorldDef();
    settings.gravity = {0.0f, -9.81f, 0.0f};
    world = b3CreateWorld(&settings);
  }

  ~Impl() {
    if (B3_IS_NULL(world)) {
      return;
    }

    bodies.clear();
    b3DestroyWorld(world);
    world = b3_nullWorldId;
  }
};

PhysicsShapeDesc PhysicsShapeDesc::makeBox(const glm::vec3 &halfExtents) {
  PhysicsShapeDesc desc;
  desc.type = PhysicsShapeType::Box;
  desc.halfExtents = halfExtents;
  return desc;
}

PhysicsShapeDesc PhysicsShapeDesc::makeSphere(float radius) {
  PhysicsShapeDesc desc;
  desc.type = PhysicsShapeType::Sphere;
  desc.radius = radius;
  return desc;
}

PhysicsShapeDesc PhysicsShapeDesc::makeCapsule(float radius, float height) {
  PhysicsShapeDesc desc;
  desc.type = PhysicsShapeType::Capsule;
  desc.radius = radius;
  desc.height = height;
  return desc;
}

PhysicsWorld::PhysicsWorld() : impl_(std::make_unique<Impl>()) {}

PhysicsWorld::~PhysicsWorld() = default;

PhysicsWorld::PhysicsWorld(PhysicsWorld &&) noexcept = default;

PhysicsWorld &PhysicsWorld::operator=(PhysicsWorld &&) noexcept = default;

void PhysicsWorld::setGravity(const glm::vec3 &gravity) {
  if (impl_ != nullptr && B3_IS_NON_NULL(impl_->world)) {
    b3World_SetGravity(impl_->world, toB3Vec3(gravity));
  }
}

PhysicsBodyHandle PhysicsWorld::createBody(const PhysicsBodyDesc &desc) {
  if (impl_ == nullptr || B3_IS_NULL(impl_->world)) {
    return {};
  }
  if (!isValidShapeDesc(desc.shape)) {
    return {};
  }

  b3BodyDef bodyDef = b3DefaultBodyDef();
  bodyDef.type = toB3BodyType(desc.type);
  bodyDef.position = toB3Vec3(desc.position);
  bodyDef.rotation = toB3Quat(desc.rotation);
  bodyDef.linearVelocity = toB3Vec3(desc.linearVelocity);
  bodyDef.linearDamping = desc.linearDamping;
  bodyDef.angularDamping = desc.angularDamping;

  b3BodyId body = b3CreateBody(impl_->world, &bodyDef);
  if (!b3Body_IsValid(body)) {
    return {};
  }

  b3ShapeDef shapeDef = b3DefaultShapeDef();
  shapeDef.density = desc.type == PhysicsBodyType::Dynamic ? 1.0f : 0.0f;
  shapeDef.filter.categoryBits = desc.categoryBits;
  shapeDef.filter.maskBits = desc.maskBits;

  b3ShapeId shape = b3_nullShapeId;
  switch (desc.shape.type) {
  case PhysicsShapeType::Box:
  {
    const b3BoxHull box = b3MakeBoxHull(desc.shape.halfExtents.x,
                                        desc.shape.halfExtents.y,
                                        desc.shape.halfExtents.z);
    shape = b3CreateHullShape(body, &shapeDef, &box.base);
    break;
  }
  case PhysicsShapeType::Sphere:
  {
    const b3Sphere sphere{{0.0f, 0.0f, 0.0f}, desc.shape.radius};
    shape = b3CreateSphereShape(body, &shapeDef, &sphere);
    break;
  }
  case PhysicsShapeType::Capsule:
  {
    const float halfHeight = desc.shape.height * 0.5f;
    const b3Capsule capsule{{0.0f, -halfHeight, 0.0f},
                            {0.0f, halfHeight, 0.0f},
                            desc.shape.radius};
    shape = b3CreateCapsuleShape(body, &shapeDef, &capsule);
    break;
  }
  }
  if (!b3Shape_IsValid(shape)) {
    b3DestroyBody(body);
    return {};
  }

  if (desc.type == PhysicsBodyType::Dynamic) {
    b3Body_ApplyMassFromShapes(body);
  }

  const std::size_t id = impl_->nextBodyId++;
  auto entry = std::make_unique<Impl::BodyEntry>(
      Impl::BodyEntry{.engineId = id,
                      .body = body,
                      .shape = shape,
                      .shapeDesc = desc.shape,
                      .shapeType = desc.shape.type});
  b3Body_SetUserData(body, entry.get());
  impl_->bodies.emplace(id, std::move(entry));
  return PhysicsBodyHandle{id};
}

void PhysicsWorld::destroyBody(PhysicsBodyHandle handle) {
  if (impl_ == nullptr || !handle.valid()) {
    return;
  }

  const auto it = impl_->bodies.find(handle.value);
  if (it == impl_->bodies.end()) {
    return;
  }

  if (b3Body_IsValid(it->second->body)) {
    b3DestroyBody(it->second->body);
  }
  impl_->bodies.erase(it);
}

void PhysicsWorld::step(float timeStep) {
  if (impl_ != nullptr && B3_IS_NON_NULL(impl_->world)) {
    b3World_Step(impl_->world, timeStep, 4);
  }
}

void PhysicsWorld::setBodyTransform(PhysicsBodyHandle handle,
                                    const glm::vec3 &position,
                                    const glm::quat &rotation) {
  if (impl_ == nullptr || !handle.valid()) {
    return;
  }

  const auto it = impl_->bodies.find(handle.value);
  if (it == impl_->bodies.end() || !b3Body_IsValid(it->second->body)) {
    return;
  }

  b3Body_SetTransform(it->second->body, toB3Vec3(position), toB3Quat(rotation));
}

void PhysicsWorld::setLinearVelocity(PhysicsBodyHandle handle,
                                     const glm::vec3 &velocity) {
  if (impl_ == nullptr || !handle.valid()) {
    return;
  }

  const auto it = impl_->bodies.find(handle.value);
  if (it == impl_->bodies.end() || !b3Body_IsValid(it->second->body)) {
    return;
  }

  b3Body_SetLinearVelocity(it->second->body, toB3Vec3(velocity));
}

PhysicsBodyState PhysicsWorld::getBodyState(PhysicsBodyHandle handle) const {
  if (impl_ == nullptr || !handle.valid()) {
    return {};
  }

  const auto it = impl_->bodies.find(handle.value);
  if (it == impl_->bodies.end() || !b3Body_IsValid(it->second->body)) {
    return {};
  }

  return PhysicsBodyState{
      .position = toGlm(b3Body_GetPosition(it->second->body)),
      .rotation = toGlm(b3Body_GetRotation(it->second->body)),
      .linearVelocity = toGlm(b3Body_GetLinearVelocity(it->second->body)),
  };
}

PhysicsRaycastHit PhysicsWorld::raycast(const glm::vec3 &start,
                                        const glm::vec3 &end,
                                        unsigned short categoryMaskBits) const {
  if (impl_ == nullptr || B3_IS_NULL(impl_->world)) {
    return {};
  }

  b3QueryFilter filter = b3DefaultQueryFilter();
  filter.categoryBits = 0xFFFF;
  filter.maskBits = categoryMaskBits;
  const b3Vec3 translation = toB3Vec3(end - start);
  const b3RayResult result =
      b3World_CastRayClosest(impl_->world, toB3Vec3(start), translation, filter);
  if (!result.hit || !b3Shape_IsValid(result.shapeId)) {
    return {};
  }

  PhysicsRaycastHit hit;
  hit.hasHit = true;
  hit.point = toGlmPos(result.point);
  hit.normal = toGlm(result.normal);
  hit.fraction = result.fraction;

  const b3BodyId body = b3Shape_GetBody(result.shapeId);
  const auto *entry = static_cast<const Impl::BodyEntry *>(b3Body_GetUserData(body));
  if (entry != nullptr) {
    hit.body = PhysicsBodyHandle{entry->engineId};
  }
  return hit;
}

void PhysicsWorld::setDebugRenderingEnabled(bool enabled) {
  if (impl_ != nullptr) {
    impl_->debugRenderingEnabled = enabled;
  }
}

bool PhysicsWorld::isDebugRenderingEnabled() const {
  return impl_ != nullptr && impl_->debugRenderingEnabled;
}

void PhysicsWorld::setDebugRenderSettings(
    const PhysicsDebugRenderSettings &settings) {
  if (impl_ == nullptr) {
    return;
  }

  impl_->debugSettings = settings;
}

PhysicsDebugRenderSettings PhysicsWorld::getDebugRenderSettings() const {
  if (impl_ == nullptr) {
    return {};
  }
  return impl_->debugSettings;
}

std::vector<PhysicsDebugLine> PhysicsWorld::getDebugLines() const {
  std::vector<PhysicsDebugLine> lines;
  if (impl_ == nullptr || !impl_->debugRenderingEnabled) {
    return lines;
  }

  if (!impl_->debugSettings.collisionShapes &&
      !impl_->debugSettings.velocityVectors) {
    return lines;
  }

  const glm::vec4 shapeColor = debugColorToGlm(0x00ff00);
  const glm::vec4 velocityColor = debugColorToGlm(0xffff00);
  for (const auto &[_, entry] : impl_->bodies) {
    if (!b3Body_IsValid(entry->body)) {
      continue;
    }

    const glm::vec3 position = toGlm(b3Body_GetPosition(entry->body));
    const glm::quat rotation =
        glm::normalize(toGlm(b3Body_GetRotation(entry->body)));

    if (impl_->debugSettings.collisionShapes) {
      switch (entry->shapeType) {
      case PhysicsShapeType::Box:
        appendBoxLines(lines, position, rotation, entry->shapeDesc.halfExtents,
                       shapeColor);
        break;
      case PhysicsShapeType::Sphere:
        appendSphereLines(lines, position, rotation, entry->shapeDesc.radius,
                          shapeColor);
        break;
      case PhysicsShapeType::Capsule:
        appendCapsuleLines(lines, position, rotation, entry->shapeDesc.radius,
                           entry->shapeDesc.height, shapeColor);
        break;
      }
    }

    if (impl_->debugSettings.velocityVectors &&
        b3Body_GetType(entry->body) == b3_dynamicBody) {
      const glm::vec3 velocity = toGlm(b3Body_GetLinearVelocity(entry->body));
      const float velocityLength = glm::length(velocity);
      if (velocityLength > 0.0001f) {
        lines.push_back(makeDebugLine(
            position,
            position + velocity * impl_->debugSettings.velocityScale,
            velocityColor));
      }
    }
  }
  return lines;
}

} // namespace DL
