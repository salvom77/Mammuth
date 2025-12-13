#include <random>
#include <limits>

class Random {
private:
    static std::mt19937_64 generator_;
    static std::uniform_real_distribution<double> dist_double_;

public:
    // Initialize the generator only once (thread-safe)
    static void init() {
        generator_.seed(std::random_device{}());
    }

    // Generate integer between min (inclusive) and max (excluding)
    static int randInt(int min, int max) {
        if (min >= max) return min;
        std::uniform_int_distribution<int> dist(min, max - 1);
        return dist(generator_);
    }

    // Generate doubles from 0.0 (inclusive) to 1.0 (excluding)
    static double randDouble() {
        return dist_double_(generator_);
    }
};

// Static definitions
std::mt19937_64 Random::generator_{std::random_device{}()};
std::uniform_real_distribution<double> Random::dist_double_(0.0, 1.0);


