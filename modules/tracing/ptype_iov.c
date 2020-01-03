/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/string_util.h>
#include <tilck/kernel/user.h>
#include <tilck/mods/tracing.h>

bool
buf_append(char *dest, int *used, int *rem, char *str);

static bool
save_param_iov(void *data, sptr iovcnt, char *dest_buf, size_t dest_bs)
{
   struct iovec *u_iovec = data;
   struct iovec iovec[4];
   bool ok;

   ASSERT(dest_bs >= 128);

   if (iovcnt <= 0)
      return false;

   iovcnt = MIN(iovcnt, 4);

   if (copy_from_user(iovec, u_iovec, sizeof(iovec[0]) * (size_t)iovcnt))
      return false;

   for (int i = 0; i < iovcnt; i++) {

      ((sptr *)dest_buf)[i] = (sptr)iovec[i].iov_len;

      ok = ptype_buffer.save(iovec[i].iov_base,
                             (sptr)iovec[i].iov_len,
                             dest_buf + 64 + 16 * i,
                             16);

      if (!ok)
         return false;
   }

   return false;
}

static bool
__dump_param_iov(char *data,
                 sptr u_iovcnt,
                 sptr maybe_tot_data_size,
                 char *dest,
                 size_t dest_bs)
{
   int used = 0, rem = (int) dest_bs;
   sptr iovcnt = MIN(u_iovcnt, 4);
   sptr tot_rem = maybe_tot_data_size >= 0 ? maybe_tot_data_size : 16;
   char buf[32];
   bool ok;

   ASSERT(dest_bs >= 128);

   snprintk(buf, sizeof(buf), "(struct iovec[%d]) {\r\n", u_iovcnt);

   if (!buf_append(dest, &used, &rem, buf))
      return false;

   for (int i = 0; i < iovcnt; i++) {

      const sptr len = ((sptr *)data)[i];

      if (!buf_append(dest, &used, &rem, "   {base: "))
         return false;

      ok = ptype_buffer.dump_from_data(
         data + 64 + i * 16,
         MIN(len, 16),
         maybe_tot_data_size >= 0 ? MIN(tot_rem, len) : len,
         buf,
         sizeof(buf)
      );

      if (!ok)
         return false;

      if (maybe_tot_data_size >= 0)
         tot_rem -= len;

      if (!buf_append(dest, &used, &rem, buf))
         return false;

      if (!buf_append(dest, &used, &rem, ", len: "))
         return false;

      snprintk(buf, sizeof(buf), "%d", len);

      if (!buf_append(dest, &used, &rem, buf))
         return false;

      if (!buf_append(dest, &used, &rem, i < u_iovcnt-1 ? "}, \r\n" : "}"))
         return false;
   }

   if (u_iovcnt > iovcnt) {
      if (!buf_append(dest, &used, &rem, "... "))
         return false;
   }

   if (!buf_append(dest, &used, &rem, "\r\n}"))
      return false;

   return true;
}

static bool
dump_param_iov_in(char *data,
                 sptr u_iovcnt,
                 sptr unused,
                 char *dest,
                 size_t dest_bs)
{
   return __dump_param_iov(data, u_iovcnt, -1, dest, dest_bs);
}

static bool
dump_param_iov_out(char *data,
                   sptr u_iovcnt,
                   sptr real_sz,
                   char *dest,
                   size_t dest_bs)
{
   return __dump_param_iov(data, u_iovcnt, real_sz, dest, dest_bs);
}


const struct sys_param_type ptype_iov_in = {

   .name = "iov",
   .slot_size = 128,

   .save = save_param_iov,
   .dump_from_data = dump_param_iov_in,
   .dump_from_val = NULL,
};

const struct sys_param_type ptype_iov_out = {

   .name = "iov",
   .slot_size = 128,

   .save = save_param_iov,
   .dump_from_data = dump_param_iov_out,
   .dump_from_val = NULL,
};
