#ifndef SRC_NODE_PERF_H_
#define SRC_NODE_PERF_H_

#if defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#include "node.h"
#include "node_perf_common.h"
#include "env.h"
#include "base_object-inl.h"
#include "histogram-inl.h"
#include "async_wrap.h"

#include "v8.h"
#include "uv.h"

#include <bitset>
#include <string>

namespace node {
namespace performance {

using v8::FunctionCallbackInfo;
using v8::GCType;
using v8::Local;
using v8::Object;
using v8::Value;

extern const uint64_t timeOrigin;

double GetCurrentTimeInMicroseconds();

static inline const char* GetPerformanceMilestoneName(
    enum PerformanceMilestone milestone) {
  switch (milestone) {
#define V(name, label) case NODE_PERFORMANCE_MILESTONE_##name: return label;
  NODE_PERFORMANCE_MILESTONES(V)
#undef V
    default:
      UNREACHABLE();
  }
}

static inline PerformanceMilestone ToPerformanceMilestoneEnum(const char* str) {
#define V(name, label)                                                        \
  if (strcmp(str, label) == 0) return NODE_PERFORMANCE_MILESTONE_##name;
  NODE_PERFORMANCE_MILESTONES(V)
#undef V
  return NODE_PERFORMANCE_MILESTONE_INVALID;
}

static inline PerformanceEntryType ToPerformanceEntryTypeEnum(
    const char* type) {
#define V(name, label)                                                        \
  if (strcmp(type, label) == 0) return NODE_PERFORMANCE_ENTRY_TYPE_##name;
  NODE_PERFORMANCE_ENTRY_TYPES(V)
#undef V
  return NODE_PERFORMANCE_ENTRY_TYPE_INVALID;
}

class PerformanceEntry {
 public:
  static void Notify(Environment* env,
                     PerformanceEntryType type,
                     Local<Value> object);

  static void New(const FunctionCallbackInfo<Value>& args);

  PerformanceEntry(Environment* env,
                   const char* name,
                   const char* type,
                   uint64_t startTime,
                   uint64_t endTime) : env_(env),
                                       name_(name),
                                       type_(type),
                                       startTime_(startTime),
                                       endTime_(endTime) { }

  virtual ~PerformanceEntry() { }

  virtual v8::MaybeLocal<Object> ToObject() const;

  Environment* env() const { return env_; }

  const std::string& name() const { return name_; }

  const std::string& type() const { return type_; }

  PerformanceEntryType kind() {
    return ToPerformanceEntryTypeEnum(type().c_str());
  }

  double startTime() const { return startTimeNano() / 1e6; }

  double duration() const { return durationNano() / 1e6; }

  uint64_t startTimeNano() const { return startTime_ - timeOrigin; }

  uint64_t durationNano() const { return endTime_ - startTime_; }

 private:
  Environment* env_;
  const std::string name_;
  const std::string type_;
  const uint64_t startTime_;
  const uint64_t endTime_;
};

enum PerformanceGCKind {
  NODE_PERFORMANCE_GC_MAJOR = GCType::kGCTypeMarkSweepCompact,
  NODE_PERFORMANCE_GC_MINOR = GCType::kGCTypeScavenge,
  NODE_PERFORMANCE_GC_INCREMENTAL = GCType::kGCTypeIncrementalMarking,
  NODE_PERFORMANCE_GC_WEAKCB = GCType::kGCTypeProcessWeakCallbacks
};

class GCPerformanceEntry : public PerformanceEntry {
 public:
  GCPerformanceEntry(Environment* env,
                     PerformanceGCKind gckind,
                     uint64_t startTime,
                     uint64_t endTime) :
                         PerformanceEntry(env, "gc", "gc", startTime, endTime),
                         gckind_(gckind) { }

  PerformanceGCKind gckind() const { return gckind_; }

 private:
  PerformanceGCKind gckind_;
};

class HistogramBase : public BaseObject,
                      public Histogram {
 public:
  HistogramBase(
    Environment* env,
    Local<Object> wrap,
    int64_t lowest,
    int64_t highest,
    int figures = 3);
  virtual ~HistogramBase() {
    Disable();
  }

  virtual void Cleanup() {}
  virtual bool Enable() {
    return false;
  }

  virtual bool Disable() {
    Cleanup();
    return false;
  }
  virtual void ResetState() {
    Reset();
  }

  void MemoryInfo(MemoryTracker* tracker) const override {
    tracker->TrackFieldWithSize("histogram", GetMemorySize());
  }
};

class RWLHistogram : public HistogramBase,
                     public ReqWrapHistogram {
 public:
  RWLHistogram(Environment* env, Local<Object> wrap);

  bool Enable() override;
  bool Disable() override;
  void Record(int type, int64_t delta) override;
  void SetTypes(v8::Local<v8::Value> names);

  SET_MEMORY_INFO_NAME(RWLHistogram)
  SET_SELF_SIZE(RWLHistogram)

 private:
  std::bitset<AsyncWrap::PROVIDERS_LENGTH> providers_;
  bool enabled_ = false;
};

class ELDHistogram : public HistogramBase {
 public:
  ELDHistogram(Environment* env,
               Local<Object> wrap,
               int32_t resolution);

  bool RecordDelta();
  void Cleanup() override;
  bool Enable() override;
  bool Disable() override;
  void ResetState() override {
    Reset();
    prev_ = 0;
  }

  SET_MEMORY_INFO_NAME(ELDHistogram)
  SET_SELF_SIZE(ELDHistogram)

 private:
  int32_t resolution_ = 0;
  uint64_t prev_ = 0;
  uv_timer_t* timer_;
};

}  // namespace performance
}  // namespace node

#endif  // defined(NODE_WANT_INTERNALS) && NODE_WANT_INTERNALS

#endif  // SRC_NODE_PERF_H_
