/*
* @Author: dazhi
* @Date:   2022-07-29 15:00:31
* @Last Modified by:   dazhi
* @Last Modified time: 2022-08-01 10:58:18
*/


#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>

#include <stdbool.h>
#include <signal.h>

#include "mixer_scontrols.h"

static char* vol_fmt="amixer sset %s %s%d%%%c%s unmute";
static char* lchannels_fmt = ",0%%+";
static char* rchannels_fmt = "0%%+,";


static char* amixer_scontrols[]={
"'PCM'",
"'Playback De-emphasis'",
"'Capture Digital'",
"'Capture Mute'",
"'Capture Polarity'",
"'3D Mode'",
"'ALC Capture Attack Time'",
"'ALC Capture Decay Time'",
"'ALC Capture Function'",
"'ALC Capture Hold Time'",
"'ALC Capture Max PGA'",
"'ALC Capture Min PGA'",
"'ALC Capture NG'",
"'ALC Capture NG Threshold'",
"'ALC Capture NG Type'",
"'ALC Capture Target'",
"'ALC Capture ZC'",
"'Differential Mux'",
"'Diffinput Set'",
"'L PGA'",
"'Left Channel'",
"'Left Line Mux'",
"'Left Mixer Left'",
"'Left Mixer Left Bypass'",
"'Left PGA Mux'",
"'Output 1'",
"'Output 2'",
"'R PGA'",
"'Right Channel'",
"'Right Line Mux'",
"'Right Mixer Right'",
"'Right Mixer Right Bypass'",
"'Right PGA Mux'",
"'Route'",
"'ZC Timeout'"};



/*
	用于mixer设备左声道的音量调节。
	index 对应于哪个通道，能取的值：PCM，Output_1，Output_2
	value 调整的值
	opt 增加value('+')或者减少value('-'),0或其他值，表示直接设置音量为value
 */
void amixer_vol_left_control(e_amixer_scontrol_index_t index,unsigned int value,char opt)
{
	char cmd[256] = {};

	if(opt != '+' && opt != '-' ) //音量增加
		opt = ' ';

	snprintf(cmd,sizeof cmd,vol_fmt,amixer_scontrols[index],"",value,opt,lchannels_fmt);

	printf("cmd = %s\n",cmd);

//	system(cmd);
}

/*
	用于mixer设备右声道的音量调节。
	index 对应于哪个通道，能取的值：PCM，Output_1，Output_2
	value 调整的值
	opt 增加value('+')或者减少value('-'),0或其他值，表示直接设置音量为value
 */
void amixer_vol_right_control(e_amixer_scontrol_index_t index,unsigned int value,char opt)
{
	char cmd[256] = {};

	if(opt != '+' && opt != '-' ) //音量增加
		opt = ' ';

	snprintf(cmd,sizeof cmd,vol_fmt,amixer_scontrols[index],rchannels_fmt,value,opt,"");

	printf("right cmd = %s\n",cmd);

//	system(cmd);
}

/*
	用于mixer设备的音量调节,两个声道同时作用。
	index 对应于哪个通道，能取的值：PCM，Output_1，Output_2
	value 调整的值
	opt 增加value('+')或者减少value('-'),0或其他值，表示直接设置音量为value
 */
void amixer_vol_control(e_amixer_scontrol_index_t index,unsigned int value,char opt)
{
	char cmd[256] = {};

	if(opt != '+' && opt != '-' ) //音量增加
		opt = ' ';

	snprintf(cmd,sizeof cmd,vol_fmt,amixer_scontrols[index],"",value,opt,"");

	printf("right cmd = %s\n",cmd);

//	system(cmd);
}


//参数 index 用来表示哪个输入通道 
void amixer_set_input_channel(e_amixer_scontrol_index_t index)
{
//	char cmd[256] = {};

//	snprintf(cmd,sizeof cmd,vol_fmt,amixer_scontrols[index],"",value,opt,"");

//	printf("right cmd = %s\n",cmd);

//	system(cmd);
}



#if 0
int main(void)
{

	amixer_vol_left_control(Output_2,10,'+');
	amixer_vol_left_control(Output_2,10,'-');
	amixer_vol_left_control(Output_2,10,0);

	amixer_vol_right_control(Output_2,10,'+');
	amixer_vol_right_control(Output_2,10,'-');
	amixer_vol_right_control(Output_2,10,0);


	amixer_vol_control(Output_2,10,'+');
	amixer_vol_control(Output_2,10,'-');
	amixer_vol_control(Output_2,10,0);


	amixer_vol_left_control(PCM,10,'+');
	amixer_vol_left_control(PCM,10,'-');
	amixer_vol_left_control(PCM,10,0);

	amixer_vol_right_control(PCM,10,'+');
	amixer_vol_right_control(PCM,10,'-');
	amixer_vol_right_control(PCM,10,0);


	amixer_vol_control(PCM,10,'+');
	amixer_vol_control(PCM,10,'-');
	amixer_vol_control(PCM,10,0);

	amixer_vol_left_control(Output_1,10,'+');
	amixer_vol_left_control(Output_1,10,'-');
	amixer_vol_left_control(Output_1,10,0);

	amixer_vol_right_control(Output_1,10,'+');
	amixer_vol_right_control(Output_1,10,'-');
	amixer_vol_right_control(Output_1,10,0);


	amixer_vol_control(Output_1,10,'+');
	amixer_vol_control(Output_1,10,'-');
	amixer_vol_control(Output_1,10,0);


	return 0;
}
#endif

