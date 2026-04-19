#include "physicsworld.h"

#include <glm/gtc/constants.hpp>
#include <reactphysics3d/reactphysics3d.h>
#include <unordered_map>

namespace DL {

namespace {

reactphysics3d::Vector3 toRp3d(const glm::vec3 &value) {
  return reactphysics3d::Vector3(value.x, value.y, value.z);
}

glm::vec3 toGlm(const reactphysics3d::Vector3 &value) {
  return {value.x, value.y, value.z};
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

reactphysics3d::Quaternion toRp3d(const glm::quat &value) {
  return reactphysics3d::Quaternion(value.x, value.y, value.z, value.w);
}

glm::quat toGlm(const reactphysics3d::Quaternion &value) {
  return {value.w, value.x, value.y, value.z};
}

reactphysics3d::BodyType toRp3d(PhysicsBodyType type) {
  switch (type) {
  case PhysicsBodyType::Static:
    return reactphysics3d::BodyType::STATIC;
  case PhysicsBodyType::Kinematic:
    return reactphysics3d::BodyType::KINEMATIC;
  case PhysicsBodyType::Dynamic:
    return reactphysics3d::BodyType::DYNAMIC;
  }
  return reactphysics3d::BodyType::STATIC;
}

} // namespace

struct PhysicsWorld::Impl {
  struct BodyEntry {
    reactphysics3d::RigidBody *body = nullptr;
    reactphysics3d::CollisionShape *shape = nullptr;
    reactphysics3d::Collider *collider = nullptr;
    PhysicsShapeDesc shapeDesc;
    PhysicsShapeType shapeType = PhysicsShapeType::Box;
  };

  reactphysics3d::PhysicsCommon physicsCommon;
  reactphysics3d::PhysicsWorld *world = nullptr;
  std::unordered_map<std::size_t, BodyEntry> bodies;
  std::size_t nextBodyId = 1;
  PhysicsDebugRenderSettings debugSettings;

  Impl() {
    reactphysics3d::PhysicsWorld::WorldSettings settings;
    settings.gravity = reactphysics3d::Vector3(0.0f, -9.81f, 0.0f);
    world = physicsCommon.createPhysicsWorld(settings);
  }

  ~Impl() {
    if (world == nullptr) {
      return;
    }

    for (auto &[_, entry] : bodies) {
      if (entry.body != nullptr) {
        world->destroyRigidBody(entry.body);
      }
      destroyShape(entry);
    }
    bodies.clear();
    physicsCommon.destroyPhysicsWorld(world);
    world = nullptr;
  }

  void destroyShape(const BodyEntry &entry) {
    if (entry.shape == nullptr) {
      return;
    }

    switch (entry.shapeType) {
    case PhysicsShapeType::Box:
      physicsCommon.destroyBoxShape(
          static_cast<reactphysics3d::BoxShape *>(entry.shape));
      break;
    case PhysicsShapeType::Sphere:
      physicsCommon.destroySphereShape(
          static_cast<reactphysics3d::SphereShape *>(entry.shape));
      break;
    case PhysicsShapeType::Capsule:
      physicsCommon.destroyCapsuleShape(
          static_cast<reactphysics3d::CapsuleShape *>(entry.shape));
      break;
    }
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
  if (impl_ != nullptr && impl_->world != nullptr) {
    impl_->world->setGravity(toRp3d(gravity));
  }
}

PhysicsBodyHandle PhysicsWorld::createBody(const PhysicsBodyDesc &desc) {
  if (impl_ == nullptr || impl_->world == nullptr) {
    return {};
  }

  reactphysics3d::CollisionShape *shape = nullptr;
  switch (desc.shape.type) {
  case PhysicsShapeType::Box:
    shape = impl_->physicsCommon.createBoxShape(toRp3d(desc.shape.halfExtents));
    break;
  case PhysicsShapeType::Sphere:
    shape = impl_->physicsCommon.createSphereShape(desc.shape.radius);
    break;
  case PhysicsShapeType::Capsule:
    shape = impl_->physicsCommon.createCapsuleShape(desc.shape.radius,
                                                    desc.shape.height);
    break;
  }
  if (shape == nullptr) {
    return {};
  }

  const reactphysics3d::Transform transform(toRp3d(desc.position),
                                            toRp3d(desc.rotation));
  auto *body = impl_->world->createRigidBody(transform);
  if (body == nullptr) {
    Impl::BodyEntry failedEntry{.shape = shape, .shapeType = desc.shape.type};
    impl_->destroyShape(failedEntry);
    return {};
  }

  body->setType(toRp3d(desc.type));
  body->setLinearDamping(desc.linearDamping);
  body->setAngularDamping(desc.angularDamping);
  auto *collider =
      body->addCollider(shape, reactphysics3d::Transform::identity());
  if (collider == nullptr) {
    impl_->world->destroyRigidBody(body);
    Impl::BodyEntry failedEntry{.shape = shape, .shapeType = desc.shape.type};
    impl_->destroyShape(failedEntry);
    return {};
  }
  body->setLinearVelocity(toRp3d(desc.linearVelocity));
  body->setIsDebugEnabled(true);
  collider->setCollisionCategoryBits(desc.categoryBits);
  collider->setCollideWithMaskBits(desc.maskBits);
  if (desc.type == PhysicsBodyType::Dynamic) {
    body->updateMassPropertiesFromColliders();
  }

  const std::size_t id = impl_->nextBodyId++;
  impl_->bodies.emplace(id, Impl::BodyEntry{.body = body,
                                            .shape = shape,
                                            .collider = collider,
                                            .shapeDesc = desc.shape,
                                            .shapeType = desc.shape.type});
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

  if (it->second.body != nullptr && impl_->world != nullptr) {
    impl_->world->destroyRigidBody(it->second.body);
  }
  impl_->destroyShape(it->second);
  impl_->bodies.erase(it);
}

void PhysicsWorld::step(float timeStep) {
  if (impl_ != nullptr && impl_->world != nullptr) {
    impl_->world->update(timeStep);
  }
}

void PhysicsWorld::setBodyTransform(PhysicsBodyHandle handle,
                                    const glm::vec3 &position,
                                    const glm::quat &rotation) {
  if (impl_ == nullptr || !handle.valid()) {
    return;
  }

  const auto it = impl_->bodies.find(handle.value);
  if (it == impl_->bodies.end() || it->second.body == nullptr) {
    return;
  }

  it->second.body->setTransform(
      reactphysics3d::Transform(toRp3d(position), toRp3d(rotation)));
}

void PhysicsWorld::setLinearVelocity(PhysicsBodyHandle handle,
                                     const glm::vec3 &velocity) {
  if (impl_ == nullptr || !handle.valid()) {
    return;
  }

  const auto it = impl_->bodies.find(handle.value);
  if (it == impl_->bodies.end() || it->second.body == nullptr) {
    return;
  }

  it->second.body->setLinearVelocity(toRp3d(velocity));
}

PhysicsBodyState PhysicsWorld::getBodyState(PhysicsBodyHandle handle) const {
  if (impl_ == nullptr || !handle.valid()) {
    return {};
  }

  const auto it = impl_->bodies.find(handle.value);
  if (it == impl_->bodies.end() || it->second.body == nullptr) {
    return {};
  }

  const auto transform = it->second.body->getTransform();
  return PhysicsBodyState{
      .position = toGlm(transform.getPosition()),
      .rotation = toGlm(transform.getOrientation()),
      .linearVelocity = toGlm(it->second.body->getLinearVelocity()),
  };
}

PhysicsRaycastHit PhysicsWorld::raycast(const glm::vec3 &start,
                                        const glm::vec3 &end,
                                        unsigned short categoryMaskBits) const {
  if (impl_ == nullptr || impl_->world == nullptr) {
    return {};
  }

  struct Callback final : reactphysics3d::RaycastCallback {
    const PhysicsWorld::Impl *impl = nullptr;
    unsigned short categoryMaskBits = 0xFFFF;
    PhysicsRaycastHit hit;

    reactphysics3d::decimal notifyRaycastHit(
        const reactphysics3d::RaycastInfo &info) override {
      if (info.collider == nullptr) {
        return 1.0f;
      }

      if ((info.collider->getCollisionCategoryBits() & categoryMaskBits) == 0) {
        return 1.0f;
      }

      hit.hasHit = true;
      hit.point = toGlm(info.worldPoint);
      hit.normal = toGlm(info.worldNormal);
      hit.fraction = static_cast<float>(info.hitFraction);

      if (impl != nullptr && info.body != nullptr) {
        for (const auto &[id, entry] : impl->bodies) {
          if (entry.body == info.body) {
            hit.body = PhysicsBodyHandle{id};
            break;
          }
        }
      }

      return info.hitFraction;
    }
  };

  Callback callback;
  callback.impl = impl_.get();
  callback.categoryMaskBits = categoryMaskBits;
  impl_->world->raycast(reactphysics3d::Ray(toRp3d(start), toRp3d(end)),
                        &callback);
  return callback.hit;
}

void PhysicsWorld::setDebugRenderingEnabled(bool enabled) {
  if (impl_ != nullptr && impl_->world != nullptr) {
    impl_->world->setIsDebugRenderingEnabled(enabled);
  }
}

bool PhysicsWorld::isDebugRenderingEnabled() const {
  return impl_ != nullptr && impl_->world != nullptr &&
         impl_->world->getIsDebugRenderingEnabled();
}

void PhysicsWorld::setDebugRenderSettings(
    const PhysicsDebugRenderSettings &settings) {
  if (impl_ == nullptr || impl_->world == nullptr) {
    return;
  }

  impl_->debugSettings = settings;
  auto &debugRenderer = impl_->world->getDebugRenderer();
  debugRenderer.setIsDebugItemDisplayed(
      reactphysics3d::DebugRenderer::DebugItem::COLLISION_SHAPE,
      settings.collisionShapes);
  debugRenderer.setIsDebugItemDisplayed(
      reactphysics3d::DebugRenderer::DebugItem::CONTACT_POINT,
      settings.contactPoints);
  debugRenderer.setIsDebugItemDisplayed(
      reactphysics3d::DebugRenderer::DebugItem::CONTACT_NORMAL,
      settings.contactNormals);
  debugRenderer.setIsDebugItemDisplayed(
      reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB,
      settings.colliderAabbs);
  debugRenderer.setIsDebugItemDisplayed(
      reactphysics3d::DebugRenderer::DebugItem::COLLIDER_BROADPHASE_AABB,
      settings.broadphaseAabbs);
}

PhysicsDebugRenderSettings PhysicsWorld::getDebugRenderSettings() const {
  if (impl_ == nullptr) {
    return {};
  }
  return impl_->debugSettings;
}

std::vector<PhysicsDebugLine> PhysicsWorld::getDebugLines() const {
  std::vector<PhysicsDebugLine> lines;
  if (impl_ == nullptr || impl_->world == nullptr ||
      !impl_->world->getIsDebugRenderingEnabled()) {
    return lines;
  }

  if (!impl_->debugSettings.collisionShapes &&
      !impl_->debugSettings.velocityVectors) {
    return lines;
  }

  const glm::vec4 shapeColor = debugColorToGlm(0x00ff00);
  const glm::vec4 velocityColor = debugColorToGlm(0xffff00);
  for (const auto &[_, entry] : impl_->bodies) {
    if (entry.body == nullptr) {
      continue;
    }

    const auto transform = entry.body->getTransform();
    const glm::vec3 position = toGlm(transform.getPosition());
    const glm::quat rotation = glm::normalize(toGlm(transform.getOrientation()));

    if (impl_->debugSettings.collisionShapes) {
      switch (entry.shapeType) {
      case PhysicsShapeType::Box:
        appendBoxLines(lines, position, rotation, entry.shapeDesc.halfExtents,
                       shapeColor);
        break;
      case PhysicsShapeType::Sphere:
        appendSphereLines(lines, position, rotation, entry.shapeDesc.radius,
                          shapeColor);
        break;
      case PhysicsShapeType::Capsule:
        appendCapsuleLines(lines, position, rotation, entry.shapeDesc.radius,
                           entry.shapeDesc.height, shapeColor);
        break;
      }
    }

    if (impl_->debugSettings.velocityVectors &&
        entry.body->getType() == reactphysics3d::BodyType::DYNAMIC) {
      const glm::vec3 velocity = toGlm(entry.body->getLinearVelocity());
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
