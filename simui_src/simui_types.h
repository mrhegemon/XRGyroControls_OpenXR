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

  float l_ep;
  float l_ey;
  float l_er;

  float r_x;
  float r_y;
  float r_z;

  float r_qx;
  float r_qy;
  float r_qz;
  float r_qw;

  float r_ep;
  float r_ey;
  float r_er;

  float l_view[16];
  float r_view[16];

  float grab_val[2];

  float l_controller[16];
  float r_controller[16];
} sharedmem_data;

#ifdef __cplusplus
}
#endif