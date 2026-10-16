#include "../app/src/main/cpp/scene_builder.h"
#include "../app/src/main/cpp/scene_instance_writer.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <type_traits>

int main(){
    static_assert(std::is_same_v<Box,vk3d::arena::Box>);
    std::vector<arena_scene::Instance> direct;direct.reserve(4096);
    const auto* storage=direct.data();const auto capacity=direct.capacity();
    SceneInstanceWriter sink(direct,4096);
    std::vector<Box> reference;reference.reserve(4096);
    bool sawGlow=false,sawRotation=false;
    for(bool manual:{false,true}){
        GameSession game;game.reserveRuntimeMemory();game.manualAim=manual;
        for(int frame=0;frame<600;++frame){
            game.updateControls(.016f,.15f,-.1f,.7f,.3f,true);
            SceneBuilder::buildDynamic(game,reference);
            SceneBuilder::buildDynamic(game,sink);
            assert(reference.size()==direct.size());
            assert(direct.data()==storage && direct.capacity()==capacity);
            for(std::size_t i=0;i<reference.size();++i){
                const auto expected=vk3d::arena::instance(reference[i]);
                assert(std::memcmp(&expected,&direct[i],sizeof(expected))==0);
                sawGlow|=reference[i].emissive>0;
                sawRotation|=reference[i].yaw!=0;
            }
        }
    }
    assert(sawGlow && sawRotation);
    direct.clear();SceneInstanceWriter limited(direct,1);
    const Box valid{0,0,0,1,1,1,1,1,1};limited.push_back(valid);
    bool rejected=false;try{limited.push_back(valid);}catch(const std::length_error&){rejected=true;}
    assert(rejected && direct.size()==1);
    limited.clear();rejected=false;
    try{limited.push_back(Box{0,0,0,-1,1,1,1,1,1});}catch(const std::invalid_argument&){rejected=true;}
    assert(rejected && direct.empty());
    std::cout<<"Direct scene stream matches staged instances; storage reuse and bounds passed\n";
}
