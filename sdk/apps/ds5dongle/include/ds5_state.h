//
// Created by awalol on 2026/10/6.
//

#ifndef SDK_DS5_STATE_H
#define SDK_DS5_STATE_H

void ds5_state_set_vol(u8 spk,u8 mic);
void ds5_state_get_vol(u8* spk,u8* mic);
void ds5_state_copy_volume_state(u8* buf,u8 len);

#endif //SDK_DS5_STATE_H
