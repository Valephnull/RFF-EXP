#ifndef UTILS_ITERATION_FILTER_INCLUDE
#define UTILS_ITERATION_FILTER_INCLUDE

bool valid_iteration_sample(double iteration) {
    return iteration > 0.0 && !isnan(iteration) && !isinf(iteration);
}

double filtered_bilinear_iteration(dvec4 values, vec2 fraction) {
    dvec4 weights = dvec4((1.0 - fraction.x) * (1.0 - fraction.y),
                          fraction.x * (1.0 - fraction.y),
                          (1.0 - fraction.x) * fraction.y,
                          fraction.x * fraction.y);
    double total = 0.0;
    double weight = 0.0;
    for (int index = 0; index < 4; ++index) {
        if (valid_iteration_sample(values[index])) {
            total += values[index] * weights[index];
            weight += weights[index];
        }
    }
    return weight > 0.0 ? total / weight : 0.0;
}

#endif
