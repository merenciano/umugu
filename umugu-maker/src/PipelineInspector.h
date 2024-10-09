#ifndef __UMUGU_MAKER_PIPELINE_ISPECTOR_H__
#define __UMUGU_MAKER_PIPELINE_ISPECTOR_H__
extern "C" {
struct umugu_node;
}
namespace umumk {
class PipelineInspector {
public:
  PipelineInspector() = default;
  void Show();

private:
  void NodeWidgets(umugu_node **apNode);
};
} // namespace umumk

#endif // __UMUGU_MAKER_PIPELINE_ISPECTOR_H__
