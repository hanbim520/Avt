
// ================================================================================================
// -*- C++ -*-
// File: obj3d.cpp
// Author: Guilherme R. Lampert
// Created on: 03/11/14
// Brief: Basic loading of 3D meshes and some procedural generation as well. Used by the demos.
//
// License:
//  This source code is released under the MIT License.
//  Copyright (c) 2014 Guilherme R. Lampert.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//
// ================================================================================================

#include "obj3d.hpp"
#include "vectormath.hpp"
#include <cassert>
#include <vector>

// Built-in models as C arrays:
extern "C" {
	extern const float    sphereLowVertexes[][3];
	extern const int      sphereLowVertexesCount;
	extern const float    sphereLowNormals[][3];
	extern const int      sphereLowNormalsCount;
	extern const float    sphereLowTexCoords[][2];
	extern const int      sphereLowTexCoordsCount;
	extern const uint16_t sphereLowIndexes[];
	extern const int      sphereLowIndexesCount;

	extern const float    boxVertexes[][3];
	extern const int      boxVertexesCount;
	extern const float    boxNormals[][3];
	extern const int      boxNormalsCount;
	extern const float    boxTexCoords[][2];
	extern const int      boxTexCoordsCount;
	extern const uint16_t boxIndexes[];
	extern const int      boxIndexesCount;
} // extern C

// ======================================================

struct Triangle
{
	uint16_t vertIndex[3];

	void setIndexes(unsigned int v0, unsigned int v1, unsigned int v2)
	{
		vertIndex[0] = static_cast<uint16_t>(v0);
		vertIndex[1] = static_cast<uint16_t>(v1);
		vertIndex[2] = static_cast<uint16_t>(v2);
	}
};

// Vertex with tangent/bi-tangent and normal (oh yeah, all the good stuff ;)
struct VertexTBN
{
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty, tz;
	float bx, by, bz;
	float u, v;

	void setPosition(const Vector3 & p)
	{
		px = p[0];
		py = p[1];
		pz = p[2];
	}

	void setNormal(const Vector3 & n)
	{
		nx = n[0];
		ny = n[1];
		nz = n[2];
	}

	void setTangent(const Vector3 & t)
	{
		tx = t[0];
		ty = t[1];
		tz = t[2];
	}

	void setBiTangent(const Vector3 & b)
	{
		bx = b[0];
		by = b[1];
		bz = b[2];
	}

	void setUV(float uv[2])
	{
		u = uv[0];
		v = uv[1];
	}
};

// ======================================================

float orthonormalize(Vector3 * v)
{
	float minLength = length(v[0]);
	v[0] = normalize(v[0]);

	for (int i = 1; i < 3; ++i)
	{
		for (int j = 0; j < i; ++j)
		{
			const float d = dot(v[i], v[j]);
			v[i] -= (v[j] * d);
		}

		const float len = length(v[i]);
		v[i] = normalize(v[i]);
		if (len < minLength)
		{
			minLength = len;
		}
	}
	return minLength;
}

// ======================================================

float computeOrthogonalComplement(Vector3 * v)
{
	if (std::abs(v[0][0]) > std::abs(v[0][1]))
	{
		v[1] = Vector3(-v[0][2], 0.0f, +v[0][0]);
	}
	else
	{
		v[1] = Vector3(0.0f, +v[0][2], -v[0][1]);
	}

	v[2] = cross(v[0], v[1]);
	return orthonormalize(v);
}

// ======================================================

void createSphere(unsigned int numZSamples, unsigned int numRadialSamples, float radius,
                  std::vector<VertexTBN> & verts, std::vector<Triangle> & triangles)
{
	constexpr float twoPi = 2 * M_PI;

	unsigned int zsm1 = numZSamples - 1;
	unsigned int zsm2 = numZSamples - 2;
	unsigned int zsm3 = numZSamples - 3;

	unsigned int rsp1 = numRadialSamples + 1;
	unsigned int numVerts = zsm2 * rsp1 + 2;
	unsigned int numTriangles = 2 * zsm2 * numRadialSamples;

	float invRS   = 1.0f / static_cast<float>(numRadialSamples);
	float zFactor = 2.0f / static_cast<float>(zsm1);

	Vector3 pos, nor, basis[3];
	float tc[2];

	verts.resize(numVerts);
	triangles.resize(numTriangles);

	// Generate points on the unit circle to be used in
	// computing the mesh points on a sphere slice.
	std::vector<float> cs(rsp1), sn(rsp1);
	for (unsigned int r = 0; r < numRadialSamples; ++r)
	{
		float angle = invRS * r * twoPi;
		cs[r] = std::cos(angle);
		sn[r] = std::sin(angle);
	}

	cs[numRadialSamples] = cs[0];
	sn[numRadialSamples] = sn[0];

	// Generate the sphere itself:
	unsigned int i = 0;
	for (unsigned int z = 1; z < zsm1; ++z)
	{
		float zFraction = -1.0f + zFactor * static_cast<float>(z); // in (-1,1)
		float zValue = radius * zFraction;

		// Compute center of slice:
		Vector3 sliceCenter(0.0f, 0.0f, zValue);

		// Compute radius of slice:
		float sliceRadius = std::sqrt(std::abs((radius * radius) - (zValue * zValue)));

		// Compute slice vertexes with duplication at endpoint:
		for (unsigned int r = 0; r <= numRadialSamples; ++r, ++i)
		{
			float radialFraction = r * invRS; // in [0,1)
			Vector3 radial(cs[r], sn[r], 0.0f);
			pos = sliceCenter + sliceRadius * radial;
			nor = pos;
			nor = normalize(nor);

			basis[0] = nor;
			computeOrthogonalComplement(basis);
			tc[0] = radialFraction;
			tc[1] = 0.5f * (zFraction + 1.0f);

			verts[i].setPosition(pos);
			verts[i].setNormal(nor);
			verts[i].setTangent(basis[1]);
			verts[i].setBiTangent(basis[2]);
			verts[i].setUV(tc);
		}
	}

	// The point at the south pole:
	pos = Vector3(0.0f, 0.0f, -radius);
	nor = Vector3(0.0f, 0.0f, -1.0f);

	basis[0] = nor;
	computeOrthogonalComplement(basis);
	tc[0] = 0.5f;
	tc[1] = 0.5f;
	verts[i].setPosition(pos);
	verts[i].setNormal(nor);
	verts[i].setTangent(basis[1]);
	verts[i].setBiTangent(basis[2]);
	verts[i].setUV(tc);
	++i;

	// The point at the north pole:
	pos = Vector3(0.0f, 0.0f, radius);
	nor = Vector3(0.0f, 0.0f, 1.0f);

	basis[0] = nor;
	computeOrthogonalComplement(basis);
	tc[0] = 0.5f;
	tc[1] = 1.0f;
	verts[i].setPosition(pos);
	verts[i].setNormal(nor);
	verts[i].setTangent(basis[1]);
	verts[i].setBiTangent(basis[2]);
	verts[i].setUV(tc);

	// Generate index buffer (outside view).
	unsigned int t = 0;
	for (unsigned int z = 0, zStart = 0; z < zsm3; ++z)
	{
		unsigned int i0 = zStart;
		unsigned int i1 = i0 + 1;
		zStart += rsp1;
		unsigned int i2 = zStart;
		unsigned int i3 = i2 + 1;

		for (i = 0; i < numRadialSamples; ++i, ++i0, ++i1, ++i2, ++i3)
		{
			triangles[t++].setIndexes(i0, i1, i2);
			triangles[t++].setIndexes(i1, i3, i2);
		}
	}

	// The south pole triangles (outside view).
	unsigned int numVerticesM2 = numVerts - 2;
	for (i = 0; i < numRadialSamples; ++i, ++t)
	{
		triangles[t].setIndexes(i, numVerticesM2, i + 1);
	}

	// The north pole triangles (outside view).
	unsigned int numVerticesM1 = numVerts - 1;
	unsigned int offset = zsm3 * rsp1;
	for (i = 0; i < numRadialSamples; ++i, ++t)
	{
		triangles[t].setIndexes(i + offset, i + 1 + offset, numVerticesM1);
	}
}

// ======================================================

struct VertexSimple
{
	float px, py, pz; // Position
	float nx, ny, nz; // Normal
	float u, v;       // Texture coords
};

// ======================================================

Obj3d * create3dObject(const float vertexes[][3], const size_t numVertexes,
                       const float normals[][3],  const size_t numNormals,
                       const float uvs[][2],      const size_t numTexCoords,
                       const uint16_t indexes[],  const size_t numIndexes)
{
	assert(numVertexes != 0);

	// Right now, these must be of the same size:
	assert(numVertexes == numNormals);
	assert(numVertexes == numTexCoords);

	// Allocate 3D object struct:
	Obj3d * obj = new Obj3d();
	obj->numVertexes = static_cast<GLsizei>(numVertexes);
	obj->numIndexes  = static_cast<GLsizei>(numIndexes);

	// Set up an interleaved vertex array
	// (much more efficient for iPhone and friends):
	std::unique_ptr<VertexSimple[]> interleavedVerts(new VertexSimple[numVertexes]);

	for (size_t i = 0; i < numVertexes; ++i)
	{
		VertexSimple * vert = &interleavedVerts[i];
		vert->px = vertexes[i][0];
		vert->py = vertexes[i][1];
		vert->pz = vertexes[i][2];
		vert->nx = normals[i][0];
		vert->ny = normals[i][1];
		vert->nz = normals[i][2];
		vert->u  = uvs[i][0];
		vert->v  = uvs[i][1];
	}

	glGenBuffers(1, &obj->vb);
	glBindBuffer(GL_ARRAY_BUFFER, obj->vb);
	glBufferData(GL_ARRAY_BUFFER, numVertexes * sizeof(VertexSimple), interleavedVerts.get(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &obj->ib);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ib);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndexes * sizeof(uint16_t), indexes, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return obj;
}

// ======================================================

Obj3d * createSphereSimple()
{
	return create3dObject(
		sphereLowVertexes,  sphereLowVertexesCount,
		sphereLowNormals,   sphereLowNormalsCount,
		sphereLowTexCoords, sphereLowTexCoordsCount,
		sphereLowIndexes,   sphereLowIndexesCount);
}

// ======================================================

Obj3d * createBoxSimple()
{
	return create3dObject(
		boxVertexes,  boxVertexesCount,
		boxNormals,   boxNormalsCount,
		boxTexCoords, boxTexCoordsCount,
		boxIndexes,   boxIndexesCount);
}

// ======================================================

void draw3dObjectSimple(const Obj3d & obj)
{
	// Bind vertex buffer:
	glBindBuffer(GL_ARRAY_BUFFER, obj.vb);

	size_t vertOffset = 0;

	// Set "vertex format":
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, /* pos.xyz */ 3, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), reinterpret_cast<GLvoid *>(vertOffset));
	vertOffset += sizeof(float) * 3;

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, /* norm.xyz */ 3, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), reinterpret_cast<GLvoid *>(vertOffset));
	vertOffset += sizeof(float) * 3;

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, /* tc.uv */ 2, GL_FLOAT, GL_FALSE, sizeof(VertexSimple), reinterpret_cast<GLvoid *>(vertOffset));

	// Attribute indexes 2 and 3 are reserved for
	// tangent and bi-tangent. Not used by this path.

	// Bind index buffer:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ib);

	// Draw all triangles of the object:
	glDrawElements(GL_TRIANGLES, obj.numIndexes, GL_UNSIGNED_SHORT, nullptr);

	// Cleanup:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ======================================================

void draw3dObjectTBN(const Obj3d & obj)
{
	// Bind vertex buffer:
	glBindBuffer(GL_ARRAY_BUFFER, obj.vb);

	size_t vertOffset = 0;

	// Set "vertex format":
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, /* pos.xyz */ 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), reinterpret_cast<GLvoid *>(vertOffset));
	vertOffset += sizeof(float) * 3;

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, /* norm.xyz */ 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), reinterpret_cast<GLvoid *>(vertOffset));
	vertOffset += sizeof(float) * 3;

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, /* tangent.xyz */ 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), reinterpret_cast<GLvoid *>(vertOffset));
	vertOffset += sizeof(float) * 3;

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, /* bitangent.xyz */ 3, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), reinterpret_cast<GLvoid *>(vertOffset));
	vertOffset += sizeof(float) * 3;

	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, /* tc.uv */ 2, GL_FLOAT, GL_FALSE, sizeof(VertexTBN), reinterpret_cast<GLvoid *>(vertOffset));

	// Bind index buffer:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.ib);

	// Draw all triangles of the object:
	glDrawElements(GL_TRIANGLES, obj.numIndexes, GL_UNSIGNED_SHORT, nullptr);

	// Cleanup:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// ======================================================

Obj3d * createSphereTBN()
{
	std::vector<VertexTBN> verts;
	std::vector<Triangle>  tris;
	createSphere(40, 40, 2.0f, verts, tris);

	Obj3d * obj = new Obj3d();
	obj->numVertexes = static_cast<GLsizei>(verts.size());
	obj->numIndexes  = static_cast<GLsizei>(tris.size()) * 3;

	glGenBuffers(1, &obj->vb);
	glBindBuffer(GL_ARRAY_BUFFER, obj->vb);
	glBufferData(GL_ARRAY_BUFFER, obj->numVertexes * sizeof(VertexTBN), verts.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &obj->ib);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ib);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj->numIndexes * sizeof(uint16_t), tris.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return obj;
}
