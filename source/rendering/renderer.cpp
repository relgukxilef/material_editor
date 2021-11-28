#include "renderer.h"

renderer::renderer() {
    compiler_options.SetOptimizationLevel(
        shaderc_optimization_level_performance
    );
    compiler_options.SetAutoBindUniforms(true);
    compiler_options.SetAutoMapLocations(true);
    compiler_options.SetGenerateDebugInfo();
}
