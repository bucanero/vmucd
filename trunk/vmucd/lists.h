typedef struct game_list {
	char dir[32];
	char name[64];
	struct game_list *next;
} game_node_t;

typedef struct save_list {
	char name[13];
	char file[13];
	char desc[256];
	ssize_t size;
	struct save_list *next;
} save_node_t;

typedef struct vmu_list {
	char name[3];
	uint8 free_blocks;
	struct vmu_list *next;
} vmu_node_t;

typedef struct menu_struct {
	int top;
	int pos;
	int total;
	int maxpage;
} menu_pos_t;

typedef struct interface_struct {
	game_node_t *games;
	menu_pos_t *game_pos;
	save_node_t *saves;
	menu_pos_t *save_pos;
	vmu_node_t *vmus;
	menu_pos_t *vmu_pos;
} interface_t;

int load_game_list(game_node_t *gptr);
int load_save_list(save_node_t *sptr, char *dir);
int load_vmu_list(vmu_node_t *vptr);
void free_game_list(game_node_t *gptr);
void free_save_list(save_node_t *sptr);
void free_vmu_list(vmu_node_t *vptr);
char* get_game_directory(game_node_t *gptr, int pos);
save_node_t* get_save_ptr(save_node_t *sptr, int pos);
vmu_node_t* get_vmu_ptr(vmu_node_t *vptr, int pos);
int get_free_blocks(maple_device_t *dev);

void draw_frame(void); // screen.h

char games_lst[1024];
char saves_lst[256];
char vmus_lst[64];
char vmu_info[384];

char* get_game_directory(game_node_t *gptr, int pos) {
	game_node_t *aux=gptr;
	int i=0;

	while (i < pos) {
		aux=aux->next;
		i++;
	}
	return(aux->dir);
}

save_node_t* get_save_ptr(save_node_t *sptr, int pos) {
	save_node_t *aux=sptr;
	int i=0;

	while (i < pos) {
		aux=aux->next;
		i++;
	}
	return(aux);
}

vmu_node_t* get_vmu_ptr(vmu_node_t *vptr, int pos) {
	vmu_node_t *aux=vptr;
	int i=0;

	while (i < pos) {
		aux=aux->next;
		i++;
	}
	return(aux);
}

int load_game_list(game_node_t *gptr) {
	char *tok;
	void *buf;
	game_node_t *aux=gptr;
	int i=-1;

	fs_load("/cd/GAMELIST.TXT", &buf);
	tok=strtok((char *)buf, "=");
	while (tok != NULL) {
		strcpy(aux->dir, tok);
//		_strupr(aux->dir);
		tok=strtok(NULL, "\n");
		if (tok != NULL) {
			strcpy(aux->name, tok);
// DEBUG
//			printf("---%d---\n%s\n%s\n", i+1, aux->dir, aux->name);
// DEBUG
			tok=strtok(NULL, "=");
			aux->next=(game_node_t *)malloc(sizeof(game_node_t));
			aux=aux->next;
			i++;
		}
	}
	aux->next=NULL;
	free(buf);
	return(i);
}

int load_save_list(save_node_t *sptr, char *dir) {
	char tmp[64];
	char *tok;
	void *buf;
	save_node_t *aux=sptr;
	int i=-1;
	file_t	f;

	sprintf(tmp, "/cd/%s/SAVELIST.TXT", dir);
	fs_load(tmp, &buf);
	tok=strtok((char *)buf, "=");
	while (tok != NULL) {
		sprintf(tmp, "/cd/%s/%s", dir, tok);
//		sprintf(tmp, "/cd/%s/%s", dir, _strupr(tok));
		sprintf(vmu_info, "Loading... %s", tok);
		draw_frame();
		tok=strtok(NULL, "\n");
		if (tok != NULL) {
			strcpy(aux->desc, tok);
			f=fs_open(tmp, O_RDONLY);
			fs_seek(f, 0x50, SEEK_SET);
			fs_read(f, tmp, 8);
			sprintf(aux->file, "%.8s.VMS", tmp);
//			_strupr(aux->file);
			fs_read(f, aux->name, 13);
			fs_seek(f, 0x68, SEEK_SET);
			fs_read(f, &(aux->size), 4);
			fs_close(f);
// DEBUG
//			printf("---%d---\n%s\n%s\n%s\n%d\n", i+1, aux->name, aux->file, aux->desc, aux->size);
// DEBUG
			tok=strtok(NULL, "=");
			aux->next=(save_node_t *)malloc(sizeof(save_node_t));
			aux=aux->next;
			i++;
		}
	}
	aux->next=NULL;
	free(buf);
	return(i);
}

int load_vmu_list(vmu_node_t *vptr) {
	int i(0);
	vmu_node_t *aux=vptr;
	maple_device_t *dev=NULL;

	while ((dev=maple_enum_type(i++,MAPLE_FUNC_MEMCARD))!=NULL) {
		sprintf(aux->name, "%c%d", (97+dev->port), dev->unit);		
		aux->free_blocks=get_free_blocks(dev);
// DEBUG
//		printf("---%d---\n%s\n%d\n", i-1, aux->name, aux->free_blocks);
// DEBUG
		aux->next=(vmu_node_t *)malloc(sizeof(vmu_node_t));
		aux=aux->next;
	}
	aux->next=NULL;
	return(i-2);
}

int get_free_blocks(maple_device_t *dev) {
	uint16 buf16[256];
	int free_blocks(0),i(0);

	if(vmu_block_read(dev,255,(uint8*)buf16)!=MAPLE_EOK)
		return(-1);
	if(vmu_block_read(dev,buf16[0x46/2],(uint8*)buf16)!=MAPLE_EOK)
		return(-1);
	for(i=0;i<200;++i) 
		if(buf16[i]==0xfffc)
			free_blocks++;
	return(free_blocks);
}

void free_game_list(game_node_t *gptr) {
	game_node_t *aux;

	while (gptr != NULL) {
		aux=gptr->next;
		free(gptr);
		gptr=aux;
	}
}

void free_save_list(save_node_t *sptr) {
	save_node_t *aux;

	while (sptr != NULL) {
		aux=sptr->next;
		free(sptr);
		sptr=aux;
	}
}

void free_vmu_list(vmu_node_t *vptr) {
	vmu_node_t *aux;

	while (vptr != NULL) {
		aux=vptr->next;
		free(vptr);
		vptr=aux;
	}
}
