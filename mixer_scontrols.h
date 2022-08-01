

#ifndef __MIXER_SCONTROLS_H__
#define __MIXER_SCONTROLS_H__

typedef enum 
{
PCM = 0,
Playback_De_emphasis,
Capture_Digital,
Capture_Mute,
Capture_Polarity,
_3D_Mode,
ALC_Capture_Attack_Time,
ALC_Capture_Decay_Time,
ALC_Capture_Function,
ALC_Capture_Hold_Time,
ALC_Capture_Max_PGA,
ALC_Capture_Min_PGA,
ALC_Capture_NG,
ALC_Capture_NG_Threshold,
ALC_Capture_NG_Type,
ALC_Capture_Target,
ALC_Capture_ZC,
Differential_Mux,
Diffinput_Set,
L_PGA,
Left_Channel,
Left_Line_Mux,
Left_Mixer_Left,
Left_Mixer_Left_Bypass,
Left_PGA_Mux,
Output_1,
Output_2,
R_PGA,
Right_Channel,
Right_Line_Mux,
Right_Mixer_Right,
Right_Mixer_Right_Bypass,
Right_PGA_Mux,
Route,
ZC_Timeout	
}e_amixer_scontrol_index_t;





/*
	用于mixer设备左声道的音量调节。
	index 对应于哪个通道，能取的值：PCM，Output_1，Output_2
	value 调整的值
	opt 增加value('+')或者减少value('-'),0或其他值，表示直接设置音量为value
 */
void amixer_vol_left_control(e_amixer_scontrol_index_t index,unsigned int value,char opt);

/*
	用于mixer设备右声道的音量调节。
	index 对应于哪个通道，能取的值：PCM，Output_1，Output_2
	value 调整的值
	opt 增加value('+')或者减少value('-'),0或其他值，表示直接设置音量为value
 */
void amixer_vol_right_control(e_amixer_scontrol_index_t index,unsigned int value,char opt);


/*
	用于mixer设备的音量调节,两个声道同时作用。
	index 对应于哪个通道，能取的值：PCM，Output_1，Output_2
	value 调整的值
	opt 增加value('+')或者减少value('-'),0或其他值，表示直接设置音量为value
 */
void amixer_vol_control(e_amixer_scontrol_index_t index,unsigned int value,char opt);


#endif
