
// なんか「そのフレームにおける各レイヤのエフェクト」を返す関数を作った。でも何をビルドすればいいか分からんので、適宜デバッグしといて。

#pragma once

#include <cstdint>
#include <tuple>
#include <string>
#include <variant>
#include <memory>
#include <optional>
#include <vector>

#include <core/project_data.hpp>
#include <core/timeline_manager.hpp>

std::vector<std::vector<Clip_t>> layer_effects;
int recent_frame;

// これを読んだら、そのフレームでどのエフェクトを処理すればいいか分かる。
std::vector<Clip_t> get_frame_effects(){
	std::vector<Clip_t> layer_frame_effects = {};
	for (int i=0; i<layer_effects.size(); i++){
		for (int j=0; j<layer_effects[i].size(); j++){
			if (layer_effects[i][j].frame_start <= recent_frame && recent_frame < layer_effects[i][j].frame_end){
				if (layer_effects[i][j].clip_type == ClipType_t::Effect){
					layer_frame_effects.push_back(layer_effects[i][j]);
				}
			}
		}
	}
	return (layer_frame_effects);
}
