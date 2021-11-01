#pragma once

struct CNNInterpolator
{
    struct TPointData {
        int id;
        float distance;
    };

    struct TPoint {
        unsigned int id;
        VEC2 position = VEC2::Zero;
        float weight = 0.f;
    };

    CNNInterpolator(bool multithreaded = true);
    ~CNNInterpolator();

    void addPoint(unsigned int id, const VEC2& pos);
    void removePoint(const unsigned int id);

    std::vector<TPoint> getWeights();
    std::vector<TPoint> getWeights(VEC2 controlPoint);
    bool isPrecomputed() { return !dirty_values; };

private:

    void updateControlPoint(VEC2 cp);
    void computeDistances();
    void computeWeights(const VEC2& pos);

    static const int gridSize;
    static const float step;

    VEC2 currentControlPoint = VEC2::Zero;

    bool dirty_values = true;
    bool dirty_weights = true;
    bool use_multithread = true;

    std::vector<TPointData> values;
    std::vector<TPoint> points;
};