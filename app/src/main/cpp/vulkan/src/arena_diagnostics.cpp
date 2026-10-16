#include "arena_api.hpp"
#include <cmath>
namespace vk3d::arena {
void recordDiagnosticCopy(VkCommandBuffer cmd,CopyImage src,CopyImage dst,VkExtent2D e,
                          VkImageLayout finalLayout,VkPipelineStageFlags finalStage,VkAccessFlags finalAccess){
    if(!cmd||!src.image||!dst.image||src.image==dst.image||!e.width||!e.height||!src.stage||!dst.stage||!finalStage||src.layout==VK_IMAGE_LAYOUT_UNDEFINED||src.layout==VK_IMAGE_LAYOUT_PREINITIALIZED||finalLayout==VK_IMAGE_LAYOUT_UNDEFINED||finalLayout==VK_IMAGE_LAYOUT_PREINITIALIZED)throw std::invalid_argument("Invalid diagnostic image copy");
    VkImageMemoryBarrier b[2]{};
    for(auto& v:b){v.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;v.srcQueueFamilyIndex=v.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
    b[0].image=src.image;b[0].oldLayout=src.layout;b[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;b[0].srcAccessMask=src.access;b[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    b[1].image=dst.image;b[1].oldLayout=dst.layout;b[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b[1].srcAccessMask=dst.access;b[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,src.stage|dst.stage,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,2,b);
    VkImageCopy copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.extent={e.width,e.height,1};
    vkCmdCopyImage(cmd,src.image,b[0].newLayout,dst.image,b[1].newLayout,1,&copy);
    b[0].oldLayout=b[0].newLayout;b[0].newLayout=src.layout;b[0].srcAccessMask=VK_ACCESS_TRANSFER_READ_BIT;b[0].dstAccessMask=src.access;
    b[1].oldLayout=b[1].newLayout;b[1].newLayout=finalLayout;b[1].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b[1].dstAccessMask=finalAccess;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,src.stage|finalStage,0,0,nullptr,0,nullptr,2,b);
}
void beginTimestamp(VkCommandBuffer cmd,VkQueryPool pool,std::uint32_t first){
    if(!cmd||!pool||first==UINT32_MAX)throw std::invalid_argument("Invalid timestamp resources");
    vkCmdResetQueryPool(cmd,pool,first,2);vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,pool,first);
}
void endTimestamp(VkCommandBuffer cmd,VkQueryPool pool,std::uint32_t first){
    if(!cmd||!pool||first==UINT32_MAX)throw std::invalid_argument("Invalid timestamp resources");
    vkCmdWriteTimestamp(cmd,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,pool,first+1);
}
double timestampMilliseconds(std::uint64_t begin,std::uint64_t end,std::uint32_t bits,float period){
    if(!bits||bits>64||!std::isfinite(period)||period<=0)throw std::invalid_argument("Invalid timestamp capabilities");
    const auto mask=bits==64?UINT64_MAX:((std::uint64_t(1)<<bits)-1);
    return double((end-begin)&mask)*double(period)/1e6;
}
bool pollTimestamp(VkDevice d,VkQueryPool pool,std::uint32_t first,std::uint32_t bits,float period,double& ms){
    if(!d||!pool||first==UINT32_MAX)throw std::invalid_argument("Invalid timestamp resources");
    timestampMilliseconds(0,0,bits,period);
    struct Result {std::uint64_t timestamp,available;} r[2]{};
    const auto status=vkGetQueryPoolResults(d,pool,first,2,sizeof(r),r,sizeof(Result),VK_QUERY_RESULT_64_BIT|VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
    if(status==VK_NOT_READY)return false;
    if(status!=VK_SUCCESS)throw VulkanError(status,"timestamp results");
    if(!r[0].available||!r[1].available)return false;
    ms=timestampMilliseconds(r[0].timestamp,r[1].timestamp,bits,period);return true;
}
}
