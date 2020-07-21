#pragma once

struct VertexColor
{
	Vector3 Position;
	Color color;

	static D3D11_INPUT_ELEMENT_DESC Desc[];
	static const uint Count = 2;

	VertexColor()
		: Position(0, 0, 0)
		, color(0, 0, 0, 1)
	{}

	VertexColor(const Vector3& position, const Color& color)
		: Position(position)
		, color(color)
	{}
};

struct VertexTexture
{
	Vector3 Position;
	Vector2 Uv;

	static D3D11_INPUT_ELEMENT_DESC Desc[];
	static const uint Count = 2;

	VertexTexture()
		: Position(0, 0, 0)
		, Uv(0, 0)
	{}

	VertexTexture(const Vector3& position, const Vector2& uv)
		: Position(position)
		, Uv(uv)
	{}
};

struct VertexTextureNormal
{
	Vector3 Position;
	Vector2 Uv;
	Vector3 Normal;

	static D3D11_INPUT_ELEMENT_DESC Desc[];
	static const uint Count = 3;

	VertexTextureNormal()
		: Position(0, 0, 0)
		, Uv(0, 0)
		, Normal(0, 0, 0)
	{}

	VertexTextureNormal
	(
		const Vector3& position, 
		const Vector2& uv,
		const Vector3& normal
	)
		: Position(position)
		, Uv(uv)
		, Normal(normal)
	{}
};

struct VertexTextureNormalTangent
{
    Vector3 Position;
    Vector2 Uv;
    Vector3 Normal;
    Vector3 Tangent;

    static D3D11_INPUT_ELEMENT_DESC Desc[];
    static const uint Count = 4;

    VertexTextureNormalTangent()
        : Position(0, 0, 0)
        , Uv(0, 0)
        , Normal(0, 0, 0)
        , Tangent(0, 0, 0)
    {}

    VertexTextureNormalTangent
    (
        const Vector3& position,
        const Vector2& uv,
        const Vector3& normal,
        const Vector3& tangent
    )
        : Position(position)
        , Uv(uv)
        , Normal(normal)
        , Tangent(tangent)
    {}
};

struct VertexTextureNormalTangentBlend
{
    Vector3 Position;
    Vector2 Uv;
    Vector3 Normal;
    Vector3 Tangent;
    Vector4 Indices;
    Vector4 Weights;

    static D3D11_INPUT_ELEMENT_DESC Desc[];
    static const uint Count = 6;

    VertexTextureNormalTangentBlend()
        : Position(0, 0, 0)
        , Uv(0, 0)
        , Normal(0, 0, 0)
        , Tangent(0, 0, 0)
        , Indices(0, 0, 0, 0)
        , Weights(0, 0, 0, 0)
    {}

    VertexTextureNormalTangentBlend
    (
        const Vector3& position,
        const Vector2& uv,
        const Vector3& normal,
        const Vector3& tangent,
        const Vector4& indices,
        const Vector4& weights
    )
        : Position(position)
        , Uv(uv)
        , Normal(normal)
        , Tangent(tangent)
        , Indices(indices)
        , Weights(weights)
    {}
};

struct VertexTerrain
{
	Vector3 Position;
	Vector2 Uv;
	Vector3 Normal;
	Color color;
	Color Alpha;

	static D3D11_INPUT_ELEMENT_DESC Desc[];
	static const uint Count = 5;

	VertexTerrain()
		: Position(0, 0, 0)
		, Uv(0, 0)
		, Normal(0, 0, 0)
		, color(0, 0, 0, 0)
		, Alpha(0, 0, 0, 0)
	{}

	VertexTerrain
	(
		const Vector3& position,
		const Vector2& uv,
		const Vector3& normal,
		const Color& color,
		const Color& alpha
	)
		: Position(position)
		, Uv(uv)
		, Normal(normal)
		, color(color)
		, Alpha(alpha)
	{}
};

typedef VertexTextureNormalTangentBlend VertexModel;