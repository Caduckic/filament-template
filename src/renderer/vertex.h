#pragma once

struct Vertex {
    filament::math::float3 position;
    filament::math::float3 normal;
    filament::math::float2 uv;
};

/* example
filament::VertexBuffer* vertexBuffer =
    filament::VertexBuffer::Builder()
        .vertexCount(vertexCount)
        .bufferCount(1)

        .attribute(
            filament::VertexAttribute::POSITION,
            0,
            filament::VertexBuffer::AttributeType::FLOAT3,
            offsetof(Vertex, position),
            sizeof(Vertex)
        )

        .attribute(
            filament::VertexAttribute::TANGENTS,
            0,
            filament::VertexBuffer::AttributeType::FLOAT3,
            offsetof(Vertex, normal),
            sizeof(Vertex)
        )

        .attribute(
            filament::VertexAttribute::UV0,
            0,
            filament::VertexBuffer::AttributeType::FLOAT2,
            offsetof(Vertex, uv),
            sizeof(Vertex)
        )

        .build(*m_engine);
*/