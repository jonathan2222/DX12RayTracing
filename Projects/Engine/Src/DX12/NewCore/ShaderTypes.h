// Note: The order of the types need to match the order of the type flags!
DEF_SHADER_TYPE(Shader::TypeFlag::Vertex, "Vertex", "VertexMain", "vs", ".vs")
DEF_SHADER_TYPE(Shader::TypeFlag::Pixel, "Pixel", "PixelMain", "ps", ".ps")
DEF_SHADER_TYPE(Shader::TypeFlag::Compute, "Compute", "ComputeMain", "cs", ".cs")
DEF_SHADER_TYPE(Shader::TypeFlag::Geometry, "Geometry", "GeometryMain", "gs", ".gs")
DEF_SHADER_TYPE(Shader::TypeFlag::Domain, "Domain", "DomainMain", "ds", ".ds")
DEF_SHADER_TYPE(Shader::TypeFlag::Hull, "Hull", "HullMain", "hs", ".hs")
