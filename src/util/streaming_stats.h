#ifndef Streaming_STATS_H
#define Streaming_STATS_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

struct Centroid {
    double mean;
    double weight;
    Centroid(double m, double w) : mean(m), weight(w) {}
};

class StreamingStats {
  public:
    StreamingStats(double compression = 100, std::size_t buffer_limit = 4096)
        : compression_(compression), buffer_limit_(buffer_limit),
          totalWeight_(0), min_(std::numeric_limits<double>::infinity()),
          max_(-std::numeric_limits<double>::infinity()), sum_(0), count_(0) {}

    // Add a single value
    void add(double x) {
        // --- classical stats ---
        if (x < min_)
            min_ = x;
        if (x > max_)
            max_ = x;
        sum_ += x;
        count_ += 1;

        if (x > last_) {
            increased_ += x - last_;
        }
        if (x < last_) {
            decreased_ += last_ - x;
        }
        last_ = x;

        // --- t-digest insert ---
        buffer_.push_back(x);
        if (buffer_.size() >= buffer_limit_)
            compress();
    }

    // Classical stats
    double min() const { return min_; }
    double max() const { return max_; }
    double sum() const { return sum_; }
    long long count() const { return count_; }
    double increased() const { return increased_; }
    double decreased() const { return decreased_; }

    double mean() const { return count_ > 0 ? sum_ / count_ : NAN; }

    // Quantile via t-digest
    double quantile(double q) {
        if (count_ == 0)
            return std::numeric_limits<double>::quiet_NaN();
        if (q <= 0)
            return min();
        if (q >= 1)
            return max();

        compress(); // ensure all data merged

        double target = q * count_;
        double cumulative = 0.0;

        const size_t n = centroids_.size();
        if (n == 0)
            return std::numeric_limits<double>::quiet_NaN();
        if (n == 1)
            return centroids_[0].mean;

        for (size_t i = 0; i < n; i++) {
            double w = centroids_[i].weight;
            double prev = cumulative;
            cumulative += w;
            if (target <= cumulative) {
                if (i == 0)
                    return centroids_[0].mean;
                const auto &L = centroids_[i - 1];
                const auto &R = centroids_[i];
                double t = (target - prev) / w;
                return L.mean + t * (R.mean - L.mean);
            }
        }
        return centroids_.back().mean;
    }

  private:
    // ------------ internal t-digest details ------------
    size_t buffer_limit_;
    double compression_;
    double totalWeight_;
    std::vector<Centroid> centroids_;
    std::vector<double> buffer_;

    double min_{0};
    double max_{0};
    double sum_{0};
    long long count_{0};

    double last_{0};
    double increased_{0};
    double decreased_{0};

    inline double wlimit(double q, double totalW) const {
        double w = 4.0 * totalW * q * (1 - q) / compression_;
        return w > 1e-12 ? w : 1e-12;
    }

    void flush() {
        if (!buffer_.empty())
            compress();
    }

    void compress() {
        if (buffer_.empty())
            return;

        // convert buffer to single-point centroids
        std::sort(buffer_.begin(), buffer_.end());
        std::vector<Centroid> all;
        all.reserve(centroids_.size() + buffer_.size());

        for (auto &c : centroids_)
            all.push_back(c);

        size_t i = 0;
        while (i < buffer_.size()) {
            size_t j = i + 1;
            while (j < buffer_.size() && buffer_[j] == buffer_[i])
                ++j;
            all.push_back({buffer_[i], double(j - i)});
            i = j;
        }
        buffer_.clear();

        std::sort(all.begin(), all.end(),
                  [](auto &a, auto &b) { return a.mean < b.mean; });

        // merge pass
        std::vector<Centroid> merged;
        merged.reserve(all.size());
        double cumulative = 0.0;

        for (const auto &c : all) {
            if (merged.empty()) {
                merged.push_back(c);
                cumulative += c.weight;
                continue;
            }
            auto &last = merged.back();

            double q = (cumulative + c.weight / 2.0) / count_;

            if (q < 0)
                q = 0;
            if (q > 1)
                q = 1;

            if (last.weight + c.weight <= wlimit(q, count_)) {
                double newW = last.weight + c.weight;
                last.mean =
                    (last.mean * last.weight + c.mean * c.weight) / newW;
                last.weight = newW;
                cumulative += c.weight;
            } else {
                merged.push_back(c);
                cumulative += c.weight;
            }
        }
        centroids_.swap(merged);
    }
};

#endif