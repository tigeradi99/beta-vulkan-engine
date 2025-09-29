echo "Compiling Model Shaders..."
C:\VulkanSDK\1.3.268.0\Bin\glslc.exe assets\shaders\simple_shader.vert -o assets\shaders\simple_shader.spv
C:\VulkanSDK\1.3.268.0\Bin\glslc.exe assets\shaders\simple_fragment.frag -o assets\shaders\simple_fragment.spv

echo "Compiling Point Light Shaders..."
C:\VulkanSDK\1.3.268.0\Bin\glslc.exe assets\shaders\point_light_shader.vert -o assets\shaders\point_light_shader.spv
C:\VulkanSDK\1.3.268.0\Bin\glslc.exe assets\shaders\point_light_fragment.frag -o assets\shaders\point_light_fragment.spv

echo "Compiling shadow Shaders..."
C:\VulkanSDK\1.3.268.0\Bin\glslc.exe assets\shaders\shadow_shader.vert -o assets\shaders\shadow_shader.spv