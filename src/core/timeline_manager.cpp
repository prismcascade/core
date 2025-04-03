
// なんか「そのフレームにおける各レイヤのエフェクト」を返す関数を作った。でも何をビルドすればいいか分からんので、適宜デバッグしといて。

#include <cstdint>
#include <tuple>
#include <string>
#include <variant>
#include <memory>
#include <optional>
#include <vector>

#include <core/project_data.hpp>
#include <core/timeline_manager.hpp>

namespace PrismCascade {

// 全レイヤ、全クリップのリスト
std::vector<std::vector<Clip_t>> layers;
int recent_frame;

// これを読んだら、そのフレームでどのエフェクトを処理すればいいか分かる。
std::vector<Clip_t> get_frame_effects(){
	std::vector<Clip_t> layer_frame_effects = {};
	for (int i=0; i<layers.size(); i++){
		for (int j=0; j<layers[i].size(); j++){
			if (layers[i][j].frame_start <= recent_frame && recent_frame <= layers[i][j].frame_end){
				if (layers[i][j].clip_type == ClipType_t::Effect){
					layer_frame_effects.push_back(layers[i][j]);
				}
			}
		}
	}
	return (layer_frame_effects);
}

// clipを何処かのレイヤに追加する関数
void add_clip(Clip_t new_clip){
	if (layers.size() < new_clip.layer){
		while(layers.size() < new_clip.layer){
			std::vector<Clip_t> new_layer = {};
			new_layer.push_back(new_clip);
			layers.push_back(new_layer);
		}
	}else{
		bool flg01 = false;
		for(int i=0; i<layers[new_clip.layer].size(); i++){
			if ((layers[new_clip.layer][i].frame_start <= new_clip.frame_start && new_clip.frame_start <= layers[new_clip.layer][i].frame_end) || (layers[new_clip.layer][i].frame_start <= new_clip.frame_end && new_clip.frame_end <= layers[new_clip.layer][i].frame_end)){
				flg01 = true;
			}
		}
		if (flg01 == false){
			layers[new_clip.layer].push_back(new_clip);
		}
	}
}

}

