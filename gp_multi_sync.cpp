#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>

#include <gphoto2/gphoto2.h>
#include <gphoto2/gphoto2-camera.h>

double sInterval = 10.;

#define MAX_CAMERAS 4	//*****  Modify this when more number of cameras to be controlled. I didn't check though.
#define DO_LOG			//*****  Uncomment this if you don't want logging.

// record file name list of same time slice.
std::string sdroot = std::string("/media/3538-3238/"); //***** Modify path according to your media mount
std::ofstream ofs;

// Gphoto globals
GPContext *sCanoncontext;
Camera* sCams[MAX_CAMERAS] = { NULL, NULL , NULL, NULL}; //*****  Modify this according to MAX_CAMERAS define value.
int sNumcams = 0;
time_t sStart_time,sEnd_time;

// To detect ESC hit
int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

// libgphoto2 call back funcs
static void ctx_error_func (GPContext *context, const char *str, void *data)
{
        std::cerr << "*** Contexterror ***              " << std::endl << str << std::endl;
}

static void ctx_status_func (GPContext *context, const char *str, void *data)
{
        std::cerr << str << std::endl;
}

// config widget search
static int _lookup_widget(CameraWidget*widget, const char *key, CameraWidget **child) {
	int ret;
	ret = gp_widget_get_child_by_name (widget, key, child);
	if (ret < GP_OK)
		ret = gp_widget_get_child_by_label (widget, key, child);
	return ret;
}

int canon_enable_capture (Camera *camera, int onoff, GPContext *context) {
	CameraWidget		*widget = NULL, *child = NULL;
	CameraWidgetType	type;
	int			ret;

	ret = gp_camera_get_config (camera, &widget, context);
	if (ret < GP_OK) {
		std::cerr << "camera_get_config failed:" <<  ret << std::endl;
		return ret;
	}
	ret = _lookup_widget (widget, "capture", &child);
	if (ret < GP_OK) {
		goto out;
	}

	ret = gp_widget_get_type (child, &type);
	if (ret < GP_OK) {
		std::cerr <<  "gp_widget_get_type failed:" <<  ret << std::endl;
		goto out;
	}
	switch (type) {
        case GP_WIDGET_TOGGLE:
			break;
		default:
			std::cerr << "widget has bad type: " << type << std::endl;
			ret = GP_ERROR_BAD_PARAMETERS;
			goto out;
	}
	// Now set the toggle to the wanted value
	ret = gp_widget_set_value (child, &onoff);
	if (ret < GP_OK) {
		std::cerr << "toggling Canon capture to " << onoff << " failed with " <<  ret << std::endl;
		goto out;
	}
	// OK
	ret = gp_camera_set_config (camera, widget, context);
	if (ret < GP_OK) {
		std::cerr << "camera_set_config failed: " << ret << std::endl;
		return ret;
	}
out:
	gp_widget_free (widget);
	return ret;
}

int set_capture_target(Camera *camera, GPContext *context, int target) {
	CameraWidget		*widget = NULL, *child = NULL;
	CameraWidgetType	type;
	int			ret;
	float			rval;
	char			*mval;

	ret = gp_camera_get_config (camera, &widget, context);
	if (ret < GP_OK) {
		fprintf (stderr, "camera_get_config failed: %d\n", ret);
		return ret;
	}
	ret = _lookup_widget (widget, "capturetarget", &child);
	if (ret < GP_OK) {
		fprintf (stderr, "lookup 'capturetarget' failed: %d\n", ret);
		goto out_capture_target;
	}

	// check that this is a toggle 
	ret = gp_widget_get_type (child, &type);
	if (ret < GP_OK) {
		fprintf (stderr, "widget get type failed: %d\n", ret);
		goto out_capture_target;
	}
	switch (type) {
        case GP_WIDGET_RADIO: {
		int choices = gp_widget_count_choices (child);

		ret = gp_widget_get_value (child, &mval);
		if (ret < GP_OK) {
			fprintf (stderr, "could not get widget value: %d\n", ret);
			goto out_capture_target;
		}
		printf("Current capture target: %s\n",mval);
		
		ret = gp_widget_get_choice (child, target, (const char**)&mval);
		if (ret < GP_OK) {
			fprintf (stderr, "could not get widget choice %d: %d\n", target, ret);
			goto out_capture_target;
		}
		printf("Setting capturetarget to %s\n",mval);
		
		ret = gp_widget_set_value (child, mval);
		if (ret < GP_OK) {
			fprintf (stderr, "could not set widget value to 1: %d\n", ret);
			goto out_capture_target;
		}
		
		break;
	}
	default:
		fprintf (stderr, "widget has bad type %d\n", type);
		ret = GP_ERROR_BAD_PARAMETERS;
		goto out_capture_target;
	}
	
	ret = gp_camera_set_config (camera, widget, context);
	if (ret < GP_OK) {
		fprintf (stderr, "could not set config tree to target: %d\n", ret);
		goto out_capture_target;
	}

out_capture_target:
	gp_widget_free (widget);

	return ret;

}

// Tethering
static void camera_tether(Camera *camera, GPContext *context) {
	int fd, retval;
	CameraFile *file;
	CameraEventType	evttype;
	CameraFilePath	*path;
	void	*evtdata;
	int waittime = 2000;
	const char *name;
	
	printf("Tethering...\n");
	
	while (1) {
		retval = gp_camera_wait_for_event (camera, waittime, &evttype, &evtdata, context);
		if (retval != GP_OK)
			break;
		switch (evttype) {
		case GP_EVENT_FILE_ADDED:
			path = (CameraFilePath*)evtdata;
			printf("File added on the camera: %s/%s\n", path->folder, path->name);
#ifdef DO_LOG
			if(ofs.is_open()) {
				ofs << path->folder << "," << path->name << ",";
			}
#endif			
			return;
			break;
		case GP_EVENT_FOLDER_ADDED:
			path = (CameraFilePath*)evtdata;
			printf("Folder added on camera: %s / %s\n", path->folder, path->name);
			break;
		case GP_EVENT_CAPTURE_COMPLETE:
			printf("Capture Complete.\n");
			waittime = 100;
			break;
		case GP_EVENT_TIMEOUT:
			printf("Timeout.\n");
			return;
			break;
		case GP_EVENT_UNKNOWN:
			break;
		default:
			printf("evttype = %d?\n", evttype);
			break;
		}
	}
}

int open_camera(Camera** camera, CameraAbilitiesList* driverlist, GPPortInfoList* portlist, const char *model, const char *port) 
{
	int		ret, m, p;
	CameraAbilities	a;
	GPPortInfo	pi;

	ret = gp_camera_new (camera);
	if (ret < GP_OK) 
		return ret;

	// First lookup the model / driver
	m = gp_abilities_list_lookup_model (driverlist, model);
	if (m < GP_OK) 
		return ret;
	ret = gp_abilities_list_get_abilities (driverlist, m, &a);
	if (ret < GP_OK) 
		return ret;
	ret = gp_camera_set_abilities (*camera, a);
	if (ret < GP_OK) 
		return ret;

	// Then associate the camera with the specified port 
	p = gp_port_info_list_lookup_path (portlist, port);  
	ret = gp_port_info_list_get_info (portlist, p, &pi);  
	if (ret < GP_OK) 
		return ret;	
	ret = gp_camera_set_port_info (*camera, pi);
	if (ret < GP_OK) 
		return ret;

	return GP_OK;
}

bool CheckAndOpenCameras() {
	GPPortInfoList* portlist = NULL;
	CameraAbilitiesList* driverlist = NULL;
	CameraList* camlist = NULL;
	
	const char* name = NULL;
	const char* value = NULL;
	int i;
	
	sCanoncontext = gp_context_new();
    gp_context_set_error_func (sCanoncontext, ctx_error_func, NULL);
    gp_context_set_status_func (sCanoncontext, ctx_status_func, NULL);

	gp_port_info_list_new (&portlist);
	gp_port_info_list_load (portlist);
	gp_port_info_list_count (portlist);

	gp_abilities_list_new (&driverlist);
	gp_abilities_list_load (driverlist, sCanoncontext);

	gp_list_new (&camlist);
	gp_abilities_list_detect (driverlist, portlist, camlist, sCanoncontext);
	sNumcams = gp_list_count(camlist);

	// list cameras 
	printf("cameras detected: %d\n", sNumcams);
	for (i = 0; i < sNumcams; i++) 
	{
		gp_list_get_name  (camlist, i, &name);
		gp_list_get_value (camlist, i, &value);
		printf("camera[%d]: %s, %s\n", i + 1, name, value);
	}

	// open cameras
	for (i = 0; i < sNumcams; i++)  	{ 		
		gp_list_get_name  (camlist, i, &name); 		
		gp_list_get_value (camlist, i, &value); 		
		printf("opening camera[%d]: %s, %s...\n", i + 1, name, value); 		
		if (open_camera (&sCams[i], driverlist, portlist, name, value) < GP_OK)
		{
			printf("failed to open camera(s)\n");
			return false;
		}
	}
	
	return true;
}

bool InitCameras() {
	int retval;
	
	// Init each
	for (int i = 0; i < sNumcams; i++)  	{ 		
		retval = gp_camera_init(sCams[i], sCanoncontext);

		if (retval != GP_OK) {
			printf("Failure of camera init: %d\n", retval);
			//goto to_end;
			//exit(-1);
			return false;
		}
		canon_enable_capture(sCams[i], TRUE, sCanoncontext);
		set_capture_target(sCams[i], sCanoncontext,1); // CF card
	}
	
	return true;
}

bool TriggerCaptureCameras() {
	int retval;
	
	for (int i = 0; i < sNumcams; i++)  	{ 
		// Triggered Capture 
		retval = gp_camera_trigger_capture (sCams[i], sCanoncontext);
		if ((retval != GP_OK) && (retval != GP_ERROR) && (retval != GP_ERROR_CAMERA_BUSY)) {
			fprintf(stderr,"triggering capture had error %d @ camera %d\n", retval,i);
		}
	}
	
	return true;
}

bool TetherCameras() {
	for (int i = 0; i < sNumcams; i++)  	{ 
		camera_tether(sCams[i], sCanoncontext) ;
	}
#ifdef DO_LOG
	if(ofs.is_open())
		ofs << std::endl;
#endif
	return true;
}

bool CloseCameras() {
	for (int i = 0; i < sNumcams; i++)  	{ 		
		gp_camera_exit(sCams[i], sCanoncontext);
	}
	
	return true;
}

void SetStartTime() {
	time(&sStart_time);

}

void WaitTimer() {
	double sec;
	
	// wait interval
	while(1) {
		time(&sEnd_time);
		sec = difftime(sEnd_time,sStart_time);
		if(sec >= (sInterval - 0.5))
			break;
	}
	
}

void save_file_to_host(Camera *camera, GPContext *context,CameraFilePath &camera_file_path) {
	CameraFile* camerafile;
	camerafile = NULL;
	CameraFilePath* curpath;
	curpath = NULL;
	int fd;
	fd = 0;
	char save_filename[260];
	int iexpose;

	curpath = &camera_file_path; 			
	sprintf(save_filename,"%s",curpath->name);
	fd = open(save_filename, O_CREAT | O_WRONLY, 0644);
	gp_file_new_from_fd(&camerafile, fd);
	gp_camera_file_get(camera, curpath->folder, curpath->name, GP_FILE_TYPE_NORMAL, camerafile, context);
	gp_file_free(camerafile);
	gp_camera_file_delete(camera, curpath->folder, curpath->name, context);
	close(fd);

}

void time_to_fn(char *str, int str_size) {
    time_t timer;
    struct tm *date;

    timer = time(NULL);
    date = localtime(&timer);

    strftime(str, str_size, "%Y_%m_%d_%H_%M_%S.cvs", date);
}

void show_usage() {
	printf("usage: multi_sync [interval(sec) > 1]\n");
}

int main(int argc, char **argv) {
	
	if(argc < 2) {
		show_usage();
		return -1;
	}
	
	// Get Interval length
	std::string ss(argv[1]);
	std::istringstream is(ss);
	is >> sInterval;
	
	if(sInterval < 1.) {
		show_usage();
		return -1;
	}

	printf("Interval: %d\n", sInterval);

	
#ifdef DO_LOG
	char tmstr[256];
	time_to_fn(tmstr,255);
	sdroot += std::string(tmstr);
	printf("Log to: %s\n",sdroot.c_str());
#endif
	
	CheckAndOpenCameras();
	
	if (sNumcams < 1)
	{
		printf("No camera(s) found.\n");
		exit(-1);
	}
	
	if(!InitCameras()) {
		exit(-1);
	}
	
#ifdef DO_LOG
	// Create Log file
	CameraFilePath camera_file_path;
	ofs.open(sdroot.c_str());
#endif

	// Main loop
	for(int n = 0;;++n) {
		int c;
		
        if (kbhit()) {
			c = getchar();
            if(c == 0x1b) // ESC
            	break;
        }
		
		SetStartTime();
		TriggerCaptureCameras();
		WaitTimer();
		TetherCameras();
		
		printf("Info: Captured #%d \n",n);

	}
	
#ifdef DO_LOG
	if(ofs.is_open())
		ofs.close();
#endif

	sleep(1);
	
to_end:
	CloseCameras();
	
	printf("Bye\n");
	
	return 0;
}
