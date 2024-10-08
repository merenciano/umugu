#ifndef __UMUGU_EDITOR_PIPELINE_BUILDER_H__
#define __UMUGU_EDITOR_PIPELINE_BUILDER_H__

#include <list>
#include <umugu/umugu.h>

namespace umumk {
class PipelineBuilder {
public:
    PipelineBuilder() = default;

    void Draw();

private:
    std::list<umugu_node*> mNodes;
};
}

#endif // __UMUGU_EDITOR_PIPELINE_BUILDER_H__