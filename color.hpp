#pragma once

// color codes
#include <glm/vec3.hpp>

namespace rgb {

// basic palete colors
glm::vec3 const black{0, 0, 0};
glm::vec3 const white{1, 1, 1};
glm::vec3 const red{1, 0, 0};
glm::vec3 const lime{0, 1, 0};
glm::vec3 const blue{0, 0, 1};
glm::vec3 const yellow{1, 1, 0};
glm::vec3 const cyan{0, 1, 1};
glm::vec3 const magenta{1, 0, 1};
glm::vec3 const silver{0.7529, 0.7529, 0.7529};
glm::vec3 const gray{0.5020, 0.5020, 0.5020};
glm::vec3 const maroon{0.5020, 0, 0};
glm::vec3 const olive{0.5020, 0.5020, 0};
glm::vec3 const green{0, 0.5020, 0};
glm::vec3 const purple{0.5020, 0, 0.5020};
glm::vec3 const teal{0, 0.5020, 0.5020};
glm::vec3 const navy{0, 0, 0.5020};

namespace yellow_shades {  // www.rapidtables.com/web/color/Yellow_Color.htm

glm::vec3 const yellow{1, 1, 0};
glm::vec3 const light_yellow{1, 1, 0.8784};
glm::vec3 const lemon_chiffon{1, 0.9804, 0.8039};
glm::vec3 const light_goldenrod_yellow{0.9804, 0.9804, 0.8235};
glm::vec3 const papaya_whip{1, 0.9373, 0.8353};
glm::vec3 const moccasin{1, 0.8941, 0.7098};
glm::vec3 const peach_puff{1, 0.8549, 0.7255};
glm::vec3 const pale_goldenrod{0.9333, 0.9098, 0.6667};
glm::vec3 const khaki{0.9412, 0.9020, 0.5490};
glm::vec3 const dark_khaki{0.7412, 0.7176, 0.4196};
glm::vec3 const olive{0.5020, 0.5020, 0};
glm::vec3 const green_yellow{0.6784, 1, 0.1843};
glm::vec3 const yellow_green{0.6039, 0.8039, 0.1961};

}  // yellow_shades

namespace blue_shades {  // http://www.rapidtables.com/web/color/blue-color.htm

glm::vec3 const alice_blue{0.94118, 0.97255, 1.00000};
glm::vec3 const lavender{0.90196, 0.90196, 0.98039};
glm::vec3 const powder_blue{0.69020, 0.87843, 0.90196};
glm::vec3 const light_blue{0.67843, 0.84706, 0.90196};
glm::vec3 const light_sky_blue{0.52941, 0.80784, 0.98039};
glm::vec3 const sky_blue{0.52941, 0.80784, 0.92157};
glm::vec3 const deep_sky_blue{0.00000, 0.74902, 1.00000};
glm::vec3 const light_steel_blue{0.69020, 0.76863, 0.87059};
glm::vec3 const dodger_blue{0.11765, 0.56471, 1.00000};
glm::vec3 const cornflower_blue{0.39216, 0.58431, 0.92941};
glm::vec3 const steel_blue{0.27451, 0.50980, 0.70588};
glm::vec3 const cadet_blue{0.37255, 0.61961, 0.62745};
glm::vec3 const medium_slate_blue{0.48235, 0.40784, 0.93333};
glm::vec3 const slate_blue{0.41569, 0.35294, 0.80392};
glm::vec3 const dark_slate_blue{0.28235, 0.23922, 0.54510};
glm::vec3 const royal_blue{0.25490, 0.41176, 0.88235};
glm::vec3 const blue{0,0,1};
glm::vec3 const medium_blue{0, 0, 0.80392};
glm::vec3 const dark_blue{0, 0, 0.54510};
glm::vec3 const navy{0, 0, 0.50196};
glm::vec3 const midnight_blue{0.098039, 0.098039, 0.439216};
glm::vec3 const blue_violet{0.54118, 0.16863, 0.88627};
glm::vec3 const indigo{0.29412, 0, 0.50980};

}  // blue_shades

}  // rgb
