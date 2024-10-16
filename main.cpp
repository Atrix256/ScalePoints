#define _CRT_SECURE_NO_WARNINGS

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "math.h"

// parameters
static const float2 c_scaleDir = Normalize(float2{ 3.0f, 1.0f });
static const float c_scaleAmount = 4.0f;

static const int c_imageSize = 512;

static const float c_graphScale = 1.0f / 8.0f;

static const float c_graphMajorTicks = 1.0f;
static const float c_graphMinorTicks = 0.25f;

// calculated things
static const float2 c_scaleDirPerp = float2{ -c_scaleDir[1], c_scaleDir[0] };
static const float c_antiAliasSize = 1.5f / float(c_imageSize);

float2 UVToMath(const float2& uv)
{
	float2 ret = uv - 0.5f;
	ret[1] *= -1.0f;
	ret = ret / c_graphScale;
	return ret;
}

float SDPoint(const float2& coord, const float2& point)
{
	return Length(coord - point);
}

float SDLineSegment(const float2& coord, const float2& A, const float2& B)
{
	float2 AB = B - A;
	float ABLen = Length(AB);
	float2 ABHat = AB / ABLen;

	float2 AP = coord - A;

	float t = Clamp(Dot(AP, ABHat), 0.0f, ABLen);

	float2 closestPoint = A + ABHat * t;

	return Length(coord - closestPoint);
}

float SDAxes(const float2& coord)
{
	// handle axes lines
	float dist = std::min(std::abs(coord[0]), std::abs(coord[1]));

	// major ticks
	{
		float tick = c_graphMajorTicks;
		while (tick < 0.5f / c_graphScale)
		{
			dist = std::min(dist, SDLineSegment(coord, float2{ tick, -1.0f / (c_graphScale * 32.0f) }, float2{ tick, 1.0f / (c_graphScale * 32.0f) }));
			dist = std::min(dist, SDLineSegment(coord, float2{ -tick, -1.0f / (c_graphScale * 32.0f) }, float2{ -tick, 1.0f / (c_graphScale * 32.0f) }));

			dist = std::min(dist, SDLineSegment(coord, float2{ -1.0f / (c_graphScale * 32.0f), tick }, float2{ 1.0f / (c_graphScale * 32.0f), tick }));
			dist = std::min(dist, SDLineSegment(coord, float2{ -1.0f / (c_graphScale * 32.0f), -tick }, float2{ 1.0f / (c_graphScale * 32.0f), -tick }));

			tick += c_graphMajorTicks;
		}
	}

	// minor ticks
	{
		float tick = c_graphMinorTicks;
		while (tick < 0.5f / c_graphScale)
		{
			dist = std::min(dist, SDLineSegment(coord, float2{ tick, -1.0f / (c_graphScale * 128.0f) }, float2{ tick, 1.0f / (c_graphScale * 128.0f) }));
			dist = std::min(dist, SDLineSegment(coord, float2{ -tick, -1.0f / (c_graphScale * 128.0f) }, float2{ -tick, 1.0f / (c_graphScale * 128.0f) }));

			dist = std::min(dist, SDLineSegment(coord, float2{ -1.0f / (c_graphScale * 128.0f), tick }, float2{ 1.0f / (c_graphScale * 128.0f), tick }));
			dist = std::min(dist, SDLineSegment(coord, float2{ -1.0f / (c_graphScale * 128.0f), -tick }, float2{ 1.0f / (c_graphScale * 128.0f), -tick }));

			tick += c_graphMinorTicks;
		}
	}

	return dist;
}

void AlphaBlend(unsigned char* RGBDest, const float3& color, float alpha)
{
	if (alpha <= 0.0f)
		return;

	for (size_t i = 0; i < 3; ++i)
	{
		float oldDest = float(RGBDest[i]) / 255.0f;
		float dest = Clamp(Lerp(oldDest, color[i], alpha), 0.0f, 1.0f);
		RGBDest[i] = (unsigned char)(dest * 255.0f);
	}	
}

void CalcPixelColor(float2 uv, unsigned char* RGB, const std::vector<float2> &points1, const std::vector<float2>& points2, const float2& scaleVector, float scaleAmount)
{
	RGB[0] = 255;
	RGB[1] = 255;
	RGB[2] = 255;

	float2 coord = UVToMath(uv);

	// draw the axes
	{
		float dist = SDAxes(coord) * c_graphScale;
		dist -= 0.0015f;
		float shade = SmoothStep(c_antiAliasSize, 0.0f, dist);
		AlphaBlend(RGB, float3{ 0.1f, 0.1f, 0.1f }, shade);
	}

	// Draw the scaled scaling vector
	{
		float dist = SDLineSegment(coord, float2{ 0.0f, 0.0f }, scaleVector * scaleAmount) * c_graphScale;

		dist -= 0.001f;

		float shade = SmoothStep(c_antiAliasSize, 0.0f, dist);
		AlphaBlend(RGB, float3{ 0.1f, 0.9f, 0.1f }, shade);
	}

	// Draw the unscaled scaling vector
	{
		float dist = SDLineSegment(coord, float2{ 0.0f, 0.0f }, scaleVector) * c_graphScale;

		dist -= 0.001f;

		float shade = SmoothStep(c_antiAliasSize, 0.0f, dist);
		AlphaBlend(RGB, float3{ 0.1f, 0.5f, 0.1f }, shade);
	}

	// draw points2
	{
		float dist = FLT_MAX;
		for (const float2& p : points2)
			dist = std::min(dist, SDPoint(coord, p));

		dist *= c_graphScale;

		dist -= 0.0045f;

		float shade = SmoothStep(c_antiAliasSize, 0.0f, dist);
		AlphaBlend(RGB, float3{ 0.1f, 0.1f, 0.9f }, shade);
	}

	// draw points1
	{
		float dist = FLT_MAX;
		for (const float2& p : points1)
			dist = std::min(dist, SDPoint(coord, p));

		dist *= c_graphScale;

		dist -= 0.0045f;

		float shade = SmoothStep(c_antiAliasSize, 0.0f, dist);
		AlphaBlend(RGB, float3{ 0.9f, 0.1f, 0.1f }, shade);
	}
}

float2 TransformPoint(const float2& p)
{
	float projection = Dot(c_scaleDir, p);
	float projectionPerp = Dot(c_scaleDirPerp, p);
	projection *= c_scaleAmount;
	return c_scaleDir * projection + c_scaleDirPerp * projectionPerp;
}

int main(int argc, char** argv)
{
	// make a circle
	std::vector<float2> points(100);
	for (size_t i = 0; i < points.size(); ++i)
	{
		float percent = float(i) / float(points.size());
		points[i][0] = std::cos(percent * 2.0f * c_pi);
		points[i][1] = std::sin(percent * 2.0f * c_pi);
	}

	/*
	// make the points of a star
	std::vector<float2> points(10);
	for (int i = 0; i < 5; ++i)
	{
		static const float c_outerRadius = 1.0f;
		static const float c_innerRadius = 0.5f;

		// convex
		points[i * 2 + 0][0] = c_outerRadius * std::cos(float(i) * 2.0f * c_pi / 5.0f + c_pi / 2.0f);
		points[i * 2 + 0][1] = c_outerRadius * std::sin(float(i) * 2.0f * c_pi / 5.0f + c_pi / 2.0f);

		// concave
		points[i * 2 + 1][0] = c_innerRadius * std::cos(float(i) * 2.0f * c_pi / 5.0f + c_pi / 2.0f + 2.0f * c_pi / 10.0f);
		points[i * 2 + 1][1] = c_innerRadius * std::sin(float(i) * 2.0f * c_pi / 5.0f + c_pi / 2.0f + 2.0f * c_pi / 10.0f);
	}
	*/
	/*
	// Make a box
	std::vector<float2> points(4);
	points[0] = float2{  1.0f,  1.0f };
	points[1] = float2{  1.0f, -1.0f };
	points[2] = float2{ -1.0f, -1.0f };
	points[3] = float2{ -1.0f,  1.0f };
	*/

	// transform the points
	std::vector<float2> pointsTransformed(points.size());
	for (size_t i = 0; i < pointsTransformed.size(); ++i)
		pointsTransformed[i] = TransformPoint(points[i]);

	// allocate the pixels for the image
	std::vector<unsigned char> pixels(c_imageSize * c_imageSize * 3, 0);

	// render each pixel individually, like how a full screen pixel shader works.
	for (int iy = 0; iy < c_imageSize; ++iy)
	{
		float v = (float(iy) + 0.5f) / float(c_imageSize);
		for (int ix = 0; ix < c_imageSize; ++ix)
		{
			float u = (float(ix) + 0.5f) / float(c_imageSize);

			CalcPixelColor(float2{ u, v }, &pixels[(iy * c_imageSize + ix) * 3], points, pointsTransformed, c_scaleDir, c_scaleAmount);
		}
	}

	// write the results out
	stbi_write_png("out.png", c_imageSize, c_imageSize, 3, pixels.data(), 0);
}
