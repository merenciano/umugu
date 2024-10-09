#ifndef __UMUGU_EDITOR_PIPELINE_BUILDER_H__
#define __UMUGU_EDITOR_PIPELINE_BUILDER_H__

#include <umugu/umugu.h>

#include <list>

namespace umumk {
class PipelineBuilder {
public:
  PipelineBuilder() = default;

  void Show();

private:
  std::list<umugu_node *> mNodes;
};
} // namespace umumk

#endif // __UMUGU_EDITOR_PIPELINE_BUILDER_H__
