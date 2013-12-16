#pragma once

#include <allegro.h>
#include <math.h>
#include "SelectionBuffer.h"
#include "types.h"

#define RENDERER_STACK_DEPTH 10

class Renderer
{
public:
	Renderer()
	{
		MatrixStack[0] = identity_matrix_f;
		MatrixStackDepth = 0;
	}

	void PushT(const Vec2F& pos)
	{
		MATRIX_f mat;

		get_translation_matrix_f(&mat, pos[0], pos[1], 0.0f);

		PushMul(mat);
	}

	void PushR(float angle)
	{
		MATRIX_f mat;

		get_z_rotate_matrix_f(&mat, RAD2ALLEG(angle));

		PushMul(mat);
	}

	void PushS(float x, float y)
	{
		MATRIX_f mat;

		get_scaling_matrix_f(&mat, x, y, 1.0f);

		PushMul(mat);
	}

	void PushMul(MATRIX_f& mat)
	{
		const MATRIX_f& mold = Get();

		MatrixStackDepth++;

		matrix_mul_f(&mat, &mold, &MatrixStack[MatrixStackDepth]);
	}

	void Pop()
	{
		MatrixStackDepth--;
	}

	const MATRIX_f& Get() const
	{
		return MatrixStack[MatrixStackDepth];
	}

	Vec2F Project(const Vec2F& p) const
	{
		const MATRIX_f& mat = Get();

		float xyz[3];

		apply_matrix_f(&mat,
			p[0],
			p[1],
			0.0f,
			&xyz[0],
			&xyz[1],
			&xyz[2]);

		return Vec2F(xyz[0], xyz[1]);
	}

	void Poly(BITMAP* buffer, const std::vector<Vec2F>& points, int c) const
	{
		for (size_t i = 0; i < points.size(); ++i)
		{
			int index1 = (i + 0) % points.size();
			int index2 = (i + 1) % points.size();

			Vec2F p1 = Project(points[index1]);
			Vec2F p2 = Project(points[index2]);

			Line(
				buffer,
				p1,
				p2,
				c);
		}
	}

	void PolyFill(BITMAP* buffer, const std::vector<Vec2F>& points, int c) const
	{
		int iPoints[32];

		// todo
		if (points.size() > 16)
			return;

		for (int i = 0; i < points.size(); ++i)
		{
			Vec2F p = Project(points[i]);

			iPoints[i * 2 + 0] = p[0];
			iPoints[i * 2 + 1] = p[1];
		}

		polygon(buffer, points.size(), iPoints, c);
	}

	void Line(BITMAP* buffer, const Vec2F& p1, const Vec2F& p2, int c) const
	{
		Vec2F p1p = Project(p1);
		Vec2F p2p = Project(p2);

		line(
			buffer,
			p1p[0],
			p1p[1],
			p2p[0],
			p2p[1],
			c);
	}

	void Rect(BITMAP* buffer, const Vec2F& p1, const Vec2F& p2, int c) const
	{
		Vec2F p1p = Project(p1);
		Vec2F p2p = Project(p2);

		rect(
			buffer,
			p1p[0],
			p1p[1],
			p2p[0],
			p2p[1],
			c);
	}

	void Circle(BITMAP* buffer, const Vec2F& p, float r, int c) const
	{
		Vec2F pp = Project(p);

		circle(
			buffer,
			pp[0],
			pp[1],
			r,
			c);
	}

	void Sprite(BITMAP* buffer, Vec2F p, float r, float s, BITMAP* sprite)
	{
		Vec2F pp = Project(p);

		float scale = s / sprite->w;

		pivot_scaled_sprite(
			buffer,
			sprite,
			pp[0],
			pp[1],
			sprite->w / 2,
			sprite->h / 2,
			ftofix(RAD2ALLEG(r)),
			ftofix(scale));
	}

	//

	void Circle(SelectionBuffer* sb, const Vec2F& p, float r, int c) const
	{
		Vec2F pp = Project(p);

		sb->Scan_Circle(p, r, c);
	}

	void MaskMap(SelectionBuffer* sb, MaskMap* mask, const Vec2F& p, int c) const
	{
		Vec2F pp = Project(p);

		sb->Scan_MaskMap(mask, pp, c);
	}

	MATRIX_f MatrixStack[RENDERER_STACK_DEPTH];
	int MatrixStackDepth;
};

extern Renderer g_Renderer;
