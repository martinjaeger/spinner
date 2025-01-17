/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arm_math.h>

#include <spinner/svm/svm.h>

/*******************************************************************************
 * Private
 ******************************************************************************/

/** Value sqrt(3). */
#define SQRT_3 1.7320508075688773f

/**
 * @brief Clip a value.
 *
 * @param[in] value Value to be clipped.
 * @param[in] min Minimum value.
 * @param[in] max Maximum value.
 *
 * @return Clipped value.
 */
static inline float clip(float value, float min, float max)
{
	if (value < min)
		return min;

	if (value > max)
		return max;

	return value;
}

/**
 * @brief Obtain sector based on a, b, c vector values.
 *
 * @param[in] a a component value.
 * @param[in] b b component value.
 * @param[in] c c component value.

 * @return Sector (1...6).
 */
static uint8_t get_sector(float a, float b, float c)
{
	uint8_t sector = 0u;

	if (c <= 0) {
		if (a <= 0) {
			sector = 2u;
		} else {
			if (b <= 0) {
				sector = 6u;
			} else {
				sector = 1u;
			}
		}
	} else {
		if (a <= 0) {
			if (b <= 0) {
				sector = 4u;
			} else {
				sector = 3u;
			}
		} else {
			sector = 5u;
		}
	}

	return sector;
}

/*******************************************************************************
 * Public
 ******************************************************************************/

void svm_init(svm_t *svm)
{
	svm->sector = 0u;

	svm->duties.a = 0.0f;
	svm->duties.b = 0.0f;
	svm->duties.c = 0.0f;
	svm->duties.max = 0.0f;

	svm->d_min = 0.0f;
	svm->d_max = 1.0f;
}

void svm_set(svm_t *svm, float va, float vb)
{
	float a, b, c, mod;
	float x, y, z;

	/* limit maximum amplitude to avoid distortions */
	(void)arm_sqrt_f32(va * va + vb * vb, &mod);
	if (mod > SQRT_3 / 2.0f) {
		va = va / mod * (SQRT_3 / 2.0f);
		vb = vb / mod * (SQRT_3 / 2.0f);
	}

	a = va - 1.0f / SQRT_3 * vb;
	b = 2.0f / SQRT_3 * vb;
	c = -(a + b);

	svm->sector = get_sector(a, b, c);

	switch (svm->sector) {
	case 1u:
		x = a;
		y = b;
		z = 1.0f - (x + y);

		svm->duties.a = x + y + z * 0.5f;
		svm->duties.b = y + z * 0.5f;
		svm->duties.c = z * 0.5f;

		svm->duties.max = svm->duties.a;
		break;

	case 2u:
		x = -c;
		y = -a;
		z = 1.0f - (x + y);

		svm->duties.a = x + z * 0.5f;
		svm->duties.b = x + y + z * 0.5f;
		svm->duties.c = z * 0.5f;

		svm->duties.max = svm->duties.b;
		break;

	case 3u:
		x = b;
		y = c;
		z = 1.0f - (x + y);

		svm->duties.a = z * 0.5f;
		svm->duties.b = x + y + z * 0.5f;
		svm->duties.c = y + z * 0.5f;

		svm->duties.max = svm->duties.b;

		break;

	case 4u:
		x = -a;
		y = -b;
		z = 1.0f - (x + y);

		svm->duties.a = z * 0.5f;
		svm->duties.b = x + z * 0.5f;
		svm->duties.c = x + y + z * 0.5f;

		svm->duties.max = svm->duties.c;

		break;

	case 5u:
		x = c;
		y = a;
		z = 1.0f - (x + y);

		svm->duties.a = y + z * 0.5f;
		svm->duties.b = z * 0.5f;
		svm->duties.c = x + y + z * 0.5f;

		svm->duties.max = svm->duties.c;

		break;

	case 6u:
		x = -b;
		y = -c;
		z = 1.0f - (x + y);

		svm->duties.a = x + y + z * 0.5f;
		svm->duties.b = z * 0.5f;
		svm->duties.c = x + z * 0.5f;

		svm->duties.max = svm->duties.a;

		break;

	default:
		break;
	}

	svm->duties.a = clip(svm->duties.a, svm->d_min, svm->d_max);
	svm->duties.b = clip(svm->duties.b, svm->d_min, svm->d_max);
	svm->duties.c = clip(svm->duties.c, svm->d_min, svm->d_max);

	svm->duties.max = clip(svm->duties.max, svm->d_min, svm->d_max);
}
