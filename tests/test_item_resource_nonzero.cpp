#include "../src/main/jni/Hacks/Feature/ItemResource1657.h"
#include <cassert>
int main() {
    ItemResourceFields value{};
    value.FxAssetID=11;
    value.InventoryModelID=22;
    value.ModelAssetIDRaw=33;
    value.UIMiniSpriteName=1;
    value.UISmallSpriteName=2;
    value.UISpriteName=3;
    value.UISquareSpriteName=4;
    ApplyItemResourceFields(value,{0,0,0},{0,0,0,0});
    assert(value.FxAssetID==11 && value.InventoryModelID==22 && value.ModelAssetIDRaw==33);
    assert(value.UIMiniSpriteName==1 && value.UISmallSpriteName==2 && value.UISpriteName==3 && value.UISquareSpriteName==4);
    ApplyItemResourceFields(value,{101,102,103},{5,6,7,8});
    assert(value.FxAssetID==101 && value.InventoryModelID==102 && value.ModelAssetIDRaw==103);
    assert(value.UIMiniSpriteName==5 && value.UISmallSpriteName==6 && value.UISpriteName==7 && value.UISquareSpriteName==8);
}
