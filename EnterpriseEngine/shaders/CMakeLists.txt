# Build HLSL shaders
add_custom_target(shaders)

set(HLSL_SHADER_FILES VertexShader.hlsl PixelShader.hlsl GenerateMipsCS.hlsli)
set_source_files_properties(VertexShader.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(PixelShader.hlsl PROPERTIES ShaderType "ps")
set_source_files_properties(GenerateMipsCS.hlsli
        PROPERTIES
            ShaderType "cs"
            VS_SHADER_VARIABLE_NAME g_GenerateMips_CS
            VS_SHADER_OUTPUT_HEADER_FILE "${IncludeDir}/shaders/GenerateMips_CS.h")
set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "5_1")

foreach(FILE ${HLSL_SHADER_FILES})
    get_filename_component(FILE_WE ${FILE} NAME_WE)
    get_source_file_property(shadertype ${FILE} ShaderType)
    get_source_file_property(shadermodel ${FILE} ShaderModel)
    add_custom_command(TARGET shaders
            COMMAND fxc.exe /nologo /Emain /T${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,/Od,/O1> /Zi /Fo ${FullOutputDir}/${FILE_WE}.cso /Fd ${FullOutputDir}/${FILE_WE}.pdb /Fh ${IncludeDir}/shaders/${FILE_WE}.h ${FILE}
            MAIN_DEPENDENCY ${FILE}
            COMMENT "HLSL ${FILE}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM)
endforeach(FILE)