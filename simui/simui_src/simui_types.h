#ifdef __cplusplus
extern "C"
{
#endif

typedef struct sharedmem_data
{
  float l_x;
  float l_y;
  float l_z;

  float l_qx;
  float l_qy;
  float l_qz;
  float l_qw;

  float r_x;
  float r_y;
  float r_z;

  float r_qx;
  float r_qy;
  float r_qz;
  float r_qw;

  float l_view[16];
  float r_view[16];

  float grab_val[2];
  float grip_val[2];

  float l_controller[16];
  float r_controller[16];
  float gaze_mat[16];
  float gaze_vec[3];

  float l_controller_quat[4];
  float r_controller_quat[4];
  float gaze_quat[4];

  int system_button;
  int menu_button;
  int left_touch_button;
  int right_touch_button;
} sharedmem_data;

#ifdef __cplusplus
}
#endif