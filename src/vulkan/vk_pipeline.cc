#include "vk_pipeline.h"
#include "src/vulkan/vk_descriptor.h"
#include "src/vulkan/vk_context.h"
#include "src/vulkan/window.h"
#include "src/vulkan/vk_buffer.h"
#include <fstream>

static std::vector<char> read_file(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file");
  }
  size_t file_size = (size_t) file.tellg();
  std::vector<char> buffer(file_size);
  file.seekg(0);
  file.read(buffer.data(), file_size);
  file.close();

  return buffer;
}

VkShaderModule create_shader_module(Vk_Context ctx, const std::vector<char>& code) {
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
  VkShaderModule shader_module;
  if (vkCreateShaderModule(ctx.m_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module");
  }
  return shader_module;
}

void create_graphics_pipeline(Vk_Pipeline &vk_pipleline, Vk_Context &vk_context, Vk_Descriptor &descriptor)
{
  auto vert_shader_code = read_file(SHADER_DIR "vert.spv");
  auto frag_shader_code = read_file(SHADER_DIR "frag.spv");

  VkShaderModule vert_shader_module = create_shader_module(vk_context, vert_shader_code);
  VkShaderModule frag_shader_module = create_shader_module(vk_context, frag_shader_code);

  VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
  vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vert_shader_module;
  vert_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
  frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = frag_shader_module;
  frag_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

  // bind vertex data to the shader
  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto binding_description = Vertex::get_binding_description();
  auto attribute_descriptions = Vertex::get_attribute_descriptions();

  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
  vertex_input_info.pVertexBindingDescriptions = &binding_description;
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  // push constant: instance_index, vertex stage only
  VkPushConstantRange push_constant_range{};
  push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  push_constant_range.offset     = 0;
  push_constant_range.size       = sizeof(int);

  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount         = 1;
  pipeline_layout_info.pSetLayouts            = &descriptor.m_layout;
  pipeline_layout_info.pushConstantRangeCount = 1;
  pipeline_layout_info.pPushConstantRanges    = &push_constant_range;

  if (vkCreatePipelineLayout(vk_context.m_device, &pipeline_layout_info, nullptr, &vk_pipleline.m_layout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout");
  }

  VkPipelineRenderingCreateInfo pipeline_rendering_info{};
  pipeline_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  pipeline_rendering_info.colorAttachmentCount = 1;
  pipeline_rendering_info.pColorAttachmentFormats = &vk_context.m_swapchain_image_format;

  std::vector<VkDynamicState> dynamic_states = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_state.pDynamicStates = dynamic_states.data();

  VkGraphicsPipelineCreateInfo pipeline_info{};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.pNext = &pipeline_rendering_info;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.layout = vk_pipleline.m_layout;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(vk_context.m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &vk_pipleline.m_graphics) != VK_SUCCESS) {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(vk_context.m_device, frag_shader_module, nullptr);
  vkDestroyShaderModule(vk_context.m_device, vert_shader_module, nullptr);
}


namespace n_pipeline
{
  void create(Vk_Pipeline &vk_pipeline, Vk_Context &vk_context, Vk_Descriptor &descriptor)
  {
    create_graphics_pipeline(vk_pipeline, vk_context, descriptor);
    std::print("\e[0;32m" "pass: " "\e[0m" "vk pipeline\n");
  }

  void destroy(Vk_Pipeline &vk_pipeline, Vk_Context &vk_context)
  {
    if (vk_pipeline.m_graphics != VK_NULL_HANDLE)
    {
      vkDestroyPipeline(vk_context.m_device, vk_pipeline.m_graphics, nullptr);
      vk_pipeline.m_graphics = VK_NULL_HANDLE;
    }
    if (vk_pipeline.m_layout != VK_NULL_HANDLE)
    {
      vkDestroyPipelineLayout(vk_context.m_device, vk_pipeline.m_layout, nullptr);
      vk_pipeline.m_layout = VK_NULL_HANDLE;
    }
  }
}
