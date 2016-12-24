#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include "defs.h"

void newcloud(struct state *state){
	int count=0;
	for(struct cloud *cloud=state->cloudlist;cloud!=NULL;cloud=cloud->next)
		++count;
	if(count>12)
		return;

	while(count<5){
		struct cloud *cloud=malloc(sizeof(struct cloud));
		cloud->base.w=CLOUD_SIZE;
		cloud->base.h=CLOUD_SIZE;
		cloud->base.rot=0.0f;
		float xoffset=randomint(1.0f,11.0f)*(onein(2)?1.0f:-1.0f);
		float yoffset=randomint(1.0f,11.0f)*(onein(2)?1.0f:-1.0f);
		cloud->base.x=(state->player.base.x+(PLAYER_WIDTH/2.0f))+xoffset;
		cloud->base.y=(state->player.base.y+(PLAYER_HEIGHT/2.0f))+yoffset;
		cloud->next=state->cloudlist;
		state->cloudlist=cloud;
		++count;
	}
}
struct cloud *deletecloud(struct state *state,struct cloud *cloud,struct cloud *prev){
	if(prev!=NULL)prev->next=cloud->next;
	else state->cloudlist=cloud->next;
	void *temp=cloud->next;
	free(cloud);
	return temp;
}

