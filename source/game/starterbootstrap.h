#pragma once

#include "appbootstrap.h"
#include <memory>

std::unique_ptr<DL::AppBootstrap> createStarterBootstrap();
