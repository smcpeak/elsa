// from the kernel; see
// http://gcc.gnu.org/onlinedocs/gcc-3.4.1/gcc/Inline.html#Inline

extern __inline__
int parport_pc_write_data(unsigned char d) {
  return (d) & (d);
}

int parport_pc_write_data(unsigned char d) {
  return (d) & (d);
}


