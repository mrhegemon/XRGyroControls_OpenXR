#ifdef __cplusplus
extern "C"
{
#endif

typedef struct openxr_headset_data
{
  float x;
  float y;
  float z;

  float qx;
  float qy;
  float qz;
  float qw;
} openxr_headset_data;

int openxr_main();
int openxr_loop();
int openxr_cleanup();
int openxr_done();
void openxr_headset_get_data(openxr_headset_data* out);

#ifdef __cplusplus
}
#endif