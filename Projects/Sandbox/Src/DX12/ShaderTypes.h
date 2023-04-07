// Note: The order of the types need to match the order of the type flags!
DEF_SHADER_TYPE(Shader::TypeFlag::VERTEX, "Vertex", "VertexMain", "vs", ".vs")
DEF_SHADER_TYPE(Shader::TypeFlag::PIXEL, "Pixel", "PixelMain", "ps", ".ps")
DEF_SHADER_TYPE(Shader::TypeFlag::COMPUTE, "Compute", "ComputeMain", "cs", ".cs")
DEF_SHADER_TYPE(Shader::TypeFlag::GEOMETRY, "Geometry", "GeometryMain", "gs", ".gs")