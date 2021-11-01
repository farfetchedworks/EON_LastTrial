#include "mcv_platform.h"
#include "nn_interpolator.h"
#include "tbb/tbb.h"

const int CNNInterpolator::gridSize = 48;
const float CNNInterpolator::step = 1.f / CNNInterpolator::gridSize;

CNNInterpolator::CNNInterpolator(bool multithreaded)
{
	points.clear();
	values.clear();
	use_multithread = multithreaded;
}

CNNInterpolator::~CNNInterpolator()
{
	points.clear();
	values.clear();
}

void CNNInterpolator::addPoint(unsigned int id, const VEC2& pos)
{
	points.push_back({ id, pos, 0.f });
	dirty_values = dirty_weights = true;
}

void CNNInterpolator::removePoint(const unsigned int id)
{
	points.erase( std::remove_if(points.begin(), points.end(), [&id](TPoint& p) {
			return p.id == id;
	}), points.end());

	dirty_values = dirty_weights = true;
}

void CNNInterpolator::updateControlPoint(VEC2 point)
{
	if (VEC2::Distance(currentControlPoint, point) < 0.001f)
		return;

	currentControlPoint = point;
	dirty_weights = true;
}

std::vector<CNNInterpolator::TPoint> CNNInterpolator::getWeights()
{
	if (dirty_weights)
	{
		computeWeights(currentControlPoint);
		dirty_weights = false;
	}

	return points;
}

std::vector<CNNInterpolator::TPoint> CNNInterpolator::getWeights(VEC2 controlPoint)
{
	updateControlPoint(controlPoint);
	return getWeights();
}

void CNNInterpolator::computeDistances()
{
	size_t numPoints = points.size();
	int size = gridSize + 1;
	int numValues = size * size;

	if (!values.size() != numValues)
		values.resize(numValues);

	for (int i = 0; i <= gridSize; ++i)
	{
		for (int j = 0; j <= gridSize; ++j)
		{
			VEC2 itpos = VEC2(float(i) * step, float(j) * step);

			// Translate grid center to 0,0 (normalize to -1,1)
			itpos = itpos * 2.f - VEC2::One;

			// dbg("%f, %f", itpos.x, itpos.y);

			int nearest = -1;
			float min_dist = 10000000.f;

			for (int k = 0; k < numPoints; ++k)
			{
				float distance = VEC2::Distance(itpos, points[k].position);
				if (distance < min_dist)
				{
					min_dist = distance;
					nearest = k;
				}
			}

			assert(nearest != -1);
			TPointData data = { nearest, min_dist };
			int index = i * size + j;
			values[index] = data;

			// dbg("   %d: %f\n", nearest, min_dist);
		}
	}

	dirty_values = false;
}

void CNNInterpolator::computeWeights(const VEC2& pos)
{
	if (!dirty_weights || points.empty()) 
		return;

	if (dirty_values)
	{
		computeDistances();
	}

	for (auto& p : points)
		p.weight = 0.f;

	int total_inside = 0;
	int size = gridSize + 1;

	if (use_multithread)
	{
		tbb::parallel_for(tbb::blocked_range<int>(0, gridSize + 1),
		[&](tbb::blocked_range<int> _i)
		{
			tbb::parallel_for(tbb::blocked_range<int>(0, gridSize + 1),
			[&](tbb::blocked_range<int> _j)
			{
				int i = _i.begin();
				int j = _j.begin();

				VEC2 itpos = VEC2(float(i) * step, float(j) * step);

				// Translate grid center to 0,0 (normalize to -1,1)
				itpos = itpos * 2.f - VEC2::One;

				int index = i * size + j;
				TPointData data = values[index];
				float dist = VEC2::Distance(itpos, pos);
				bool is_inside = data.distance > 0 ?
					dist <= (data.distance + 1e-8f) : dist == 0;

				// Alex: don't give weights to idle in case Eon
				// is moving since this doesn't blend correctly
				if (data.id == 0 && pos != VEC2::Zero) {
					is_inside = false;
				}

				if (is_inside)
				{
					points[data.id].weight += 1.f;
					++total_inside;
				}
			});
		});
	}
	else
	{
		for (int i = 0; i <= gridSize; ++i)
		{
			for (int j = 0; j <= gridSize; ++j)
			{
				VEC2 itpos = VEC2(float(i) * step, float(j) * step);

				// Translate grid center to 0,0 (normalize to -1,1)
				itpos = itpos * 2.f - VEC2::One;

				int index = i * size + j;
				TPointData data = values[index];
				float dist = VEC2::Distance(itpos, pos);
				bool is_inside = data.distance > 0 ?
					dist <= (data.distance + 1e-8f) : dist == 0;

				// Alex: don't give weights to idle in case Eon
				// is moving since this doesn't blend correctly
				if (data.id == 0 && pos != VEC2::Zero) {
					is_inside = false;
				}

				if (is_inside)
				{
					points[data.id].weight += 1.f;
					++total_inside;
				}
			}
		}
	}

	// If any point has weights, give all to "Idle"
	if (!total_inside) {
		total_inside = 1;
		points[0].weight = 1;
	}

	// Normalize weights
	float normFactor = 1.f / float(total_inside);

	for (auto& el : points) {
		el.weight *= normFactor;
	}
}