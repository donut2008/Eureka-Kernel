 )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	w2(4);
}

static void frpw_disconnect ( PIA *pi )

{       w2(4); w0(0x20); cec4;
	w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

/* Stub logic to see if PNP string is available - used to distinguish
   between the Xilinx and ASIC implementations of the Freecom adapter.
*/

static int frpw_test_pnp ( PIA *pi )

/*  returns chip_type:   0 = Xilinx, 1 = ASIC   */

{	int olddelay, a, b;

#ifdef FRPW_HARD_RESET
        w0(0); w2(8); udelay(50); w2(0xc);   /* parallel bus reset */
        mdelay(1500);
#endif

	olddelay = pi->delay;
	pi->delay = 10;

	pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	
	w2(4); w0(4); w2(6); w2(7);
	a = r1() & 0xff; w2(4); b = r1() & 0xff;
	w2(0xc); w2(0xe); w2(4);

	pi->delay = olddelay;
        w0(pi->saved_r0);
        w2(pi->saved_r2);

	return ((~a&0x40) && (b&0x40));
} 

/* We use the pi->private to remember the result of the PNP test.
   To make this work, private = port*2 + chip.  Yes, I know it's
   a hack :-(
*/

static int frpw_test_proto( PIA *pi, char * scratch, int verbose )

{       int     j, k, r;
	int	e[2] = {0,0};

	if ((pi->private>>1) != pi->port)
	   pi->private = frpw_test_pnp(pi) + 2*pi->port;

	if (((pi->private%2) == 0) && (pi->mode > 2)) {
	   if (verbose) 
		printk("%s: frpw: Xilinx does not support mode %d\n",
			pi->device, pi->mode);
	   return 1;
	}

	if (((pi->private%2) == 1) && (pi->mode == 2)) {
	   if (verbose)
		printk("%s: frpw: ASIC does not support mode 2\n",
			pi->device);
	   return 1;
	}

	frpw_connect(pi);
	for (j=0;j<2;j++) {
                frpw_write_regr(pi,0,6,0xa0+j*0x10);
                for (k=0;k<256;k++) {
                        frpw_write_regr(pi,0,2,k^0xaa);
                        frpw_write_regr(pi,0,3,k^0x55);
                        if (frpw_read_regr(pi,0,2) != (k^0xaa)) e[j]++;
                        }
                }
	frpw_disconnect(pi);

	frpw_connect(pi);
        frpw_read_block_int(pi,scratch,512,0x10);
        r = 0;
        for (k=0;k<128;k++) if (scratch[k] != k) r++;
	frpw_disconnect(pi);

        if (verbose)  {
            printk("%s: frpw: port 0x%x, chip %ld, mode %d, test=(%d,%d,%d)\n",
                   pi->device,pi->port,(pi->private%2),pi->mode,e[0],e[1],r);
        }

        return (r || (e[0] && e[1]));
}


static void frpw_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[6] = {"4-bit","8-bit","EPP",
				   "EPP-8","EPP-16","EPP-32"};

        printk("%s: frpw %s, Freecom (%s) adapter at 0x%x, ", pi->device,
		FRPW_VERSION,((pi->private%2) == 0)?"Xilinx":"ASIC",pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol frpw = {
	.owner		= THIS_MODULE,
	.name		= "frpw",
	.max_mode	= 6,
	.epp_first	= 2,
	.default_delay	= 2,
	.max_units	= 1,
	.write_regr	= frpw_write_regr,
	.read_regr	= frpw_read_regr,
	.write_block	= frpw_write_block,
	.read_block	= frpw_read_block,
	.connect	= frpw_connect,
	.disconnect	= frpw_disconnect,
	.test_proto	= frpw_test_proto,
	.log_adapter	= frpw_log_adapter,
};

static int __init frpw_init(void)
{
	return paride_register(&frpw);
}

static void __exit frpw_exit(void)
{
	paride_unregister(&frpw);
}

MODULE_LICENSE("GPL");
module_init(frpw_init)
module_exit(frpw_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           /*
        kbic.c    (c) 1997-8  Grant R. Guenther <grant@torque.net>
                              Under the terms of the GNU General Public License.

        This is a low-level driver for the KBIC-951A and KBIC-971A
        parallel to IDE adapter chips from KingByte Information Systems.

	The chips are almost identical, however, the wakeup code 
	required for the 971A interferes with the correct operation of
        the 951A, so this driver registers itself twice, once for
	each chip.

*/

/* Changes:

        1.01    GRG 1998.05.06 init_proto, release_proto

*/

#define KBIC_VERSION      "1.01"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#define r12w()			(delay_p,inw(pi->port+1)&0xffff) 

#define j44(a,b)                ((((a>>4)&0x0f)|(b&0xf0))^0x88)
#define j53(w)                  (((w>>3)&0x1f)|((w>>4)&0xe0))


/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int  cont_map[2] = { 0x80, 0x40 };

static int kbic_read_regr( PIA *pi, int cont, int regr )

{       int     a, b, s;

        s = cont_map[cont];

	switch (pi->mode) {

	case 0: w0(regr|0x18|s); w2(4); w2(6); w2(4); w2(1); w0(8);
	        a = r1(); w0(0x28); b = r1(); w2(4);
		return j44(a,b);

	case 1: w0(regr|0x38|s); w2(4); w2(6); w2(4); w2(5); w0(8);
		a = r12w(); w2(4);
		return j53(a);

	case 2: w0(regr|0x08|s); w2(4); w2(6); w2(4); w2(0xa5); w2(0xa1);
		a = r0(); w2(4);
       		return a;

	case 3:
	case 4:
	case 5: w0(0x20|s); w2(4); w2(6); w2(4); w3(regr);
		a = r4(); b = r4(); w2(4); w2(0); w2(4);
		return a;

	}
	return -1;
}       

static void  kbic_write_regr( PIA *pi, int cont, int regr, int val)

{       int  s;

        s = cont_map[cont];

        switch (pi->mode) {

	case 0: 
        case 1:
	case 2:	w0(regr|0x10|s); w2(4); w2(6); w2(4); 
		w0(val); w2(5); w2(4);
		break;

	case 3:
	case 4:
	case 5: w0(0x20|s); w2(4); w2(6); w2(4); w3(regr);
		w4(val); w4(val);
		w2(4); w2(0); w2(4);
                break;

	}
}

static void k951_connect ( PIA *pi  )

{ 	pi->saved_r0 = r0();
        pi->saved_r2 = r2();
        w2(4); 
}

static void k951_disconnect ( PIA *pi )

{      	w0(pi->saved_r0);
        w2(pi->saved_r2);
}

#define	CCP(x)	w2(0xc4);w0(0xaa);w0(0x55);w0(0);w0(0xff);w0(0x87);\
		w0(0x78);w0(x);w2(0xc5);w2(0xc4);w0(0xff);

static void k971_connect ( PIA *pi  )

{ 	pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	CCP(0x20);
        w2(4); 
}

static void k971_disconnect ( PIA *pi )

{       CCP(0x30);
	w0(pi->saved_r0);
        w2(pi->saved_r2);
}

/* counts must be congruent to 0 MOD 4, but all known applications
   have this property.
*/

static void kbic_read_block( PIA *pi, char * buf, int count )

{       int     k, a, b;

        switch (pi->mode) {

        case 0: w0(0x98); w2(4); w2(6); w2(4);
                for (k=0;k<count/2;k++) {
			w2(1); w0(8);    a = r1();
			       w0(0x28); b = r1();
			buf[2*k]   = j44(a,b);
			w2(5);           b = r1();
			       w0(8);    a = r1();
			buf[2*k+1] = j44(a,b);
			w2(4);
                } 
                break;

        case 1: w0(0xb8); w2(4); w2(6); w2(4); 
                for (k=0;k<count/4;k++) {
                        w0(0xb8); 
			w2(4); w2(5); 
                        w0(8);    buf[4*k]   = j53(r12w());
			w0(0xb8); buf[4*k+1] = j53(r12w());
			w2(4); w2(5);
			          buf[4*k+3] = j53(r12w());
			w0(8);    buf[4*k+2] = j53(r12w());
                }
                w2(4);
                break;

        case 2: w0(0x88); w2(4); w2(6); w2(4);
                for (k=0;k<count/2;k++) {
                        w2(0xa0); w2(0xa1); buf[2*k] = r0();
                        w2(0xa5); buf[2*k+1] = r0();
                }
                w2(4);
                break;

        case 3: w0(0xa0); w2(4); w2(6); w2(4); w3(0);
                for (k=0;k<count;k++) buf[k] = r4();
                w2(4); w2(0); w2(4);
                break;

	case 4: w0(0xa0); w2(4); w2(6); w2(4); w3(0);
                for (k=0;k<count/2;k++) ((u16 *)buf)[k] = r4w();
                w2(4); w2(0); w2(4);
                break;

        case 5: w0(0xa0); w2(4); w2(6); w2(4); w3(0);
                for (k=0;k<count/4;k++) ((u32 *)buf)[k] = r4l();
                w2(4); w2(0); w2(4);
                break;


        }
}

static void kbic_write_block( PIA *pi, char * buf, int count )

{       int     k;

        switch (pi->mode) {

        case 0:
        case 1:
        case 2: w0(0x90); w2(4); w2(6); w2(4); 
		for(k=0;k<count/2;k++) {
			w0(buf[2*k+1]); w2(0); w2(4); 
			w0(buf[2*k]);   w2(5); w2(4); 
		}
		break;

        case 3: w0(0xa0); w2(4); w2(6); w2(4); w3(0);
		for(k=0;k<count/2;k++) {
			w4(buf[2*k+1]); 
                        w4(buf[2*k]);
                }
		w2(4); w2(0); w2(4);
		break;

	case 4: w0(0xa0); w2(4); w2(6); w2(4); w3(0);
                for(k=0;k<count/2;k++) w4w(pi_swab16(buf,k));
                w2(4); w2(0); w2(4);
                break;

        case 5: w0(0xa0); w2(4); w2(6); w2(4); w3(0);
                for(k=0;k<count/4;k++) w4l(pi_swab32(buf,k));
                w2(4); w2(0); w2(4);
                break;

        }

}

static void kbic_log_adapter( PIA *pi, char * scratch, 
			      int verbose, char * chip )

{       char    *mode_string[6] = {"4-bit","5/3","8-bit",
				   "EPP-8","EPP_16","EPP-32"};

        printk("%s: kbic %s, KingByte %s at 0x%x, ",
                pi->device,KBIC_VERSION,chip,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static void k951_log_adapter( PIA *pi, char * scratch, int verbose )

{	kbic_log_adapter(pi,scratch,verbose,"KBIC-951A");
}

static void k971_log_adapter( PIA *pi, char * scratch, int verbose )

{       kbic_log_adapter(pi,scratch,verbose,"KBIC-971A");
}

static struct pi_protocol k951 = {
	.owner		= THIS_MODULE,
	.name		= "k951",
	.max_mode	= 6,
	.epp_first	= 3,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= kbic_write_regr,
	.read_regr	= kbic_read_regr,
	.write_block	= kbic_write_block,
	.read_block	= kbic_read_block,
	.connect	= k951_connect,
	.disconnect	= k951_disconnect,
	.log_adapter	= k951_log_adapter,
};

static struct pi_protocol k971 = {
	.owner		= THIS_MODULE,
	.name		= "k971",
	.max_mode	= 6,
	.epp_first	= 3,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= kbic_write_regr,
	.read_regr	= kbic_read_regr,
	.write_block	= kbic_write_block,
	.read_block	= kbic_read_block,
	.connect	= k971_connect,
	.disconnect	= k971_disconnect,
	.log_adapter	= k971_log_adapter,
};

static int __init kbic_init(void)
{
	int rv;

	rv = paride_register(&k951);
	if (rv < 0)
		return rv;
	rv = paride_register(&k971);
	if (rv < 0)
		paride_unregister(&k951);
	return rv;
}

static void __exit kbic_exit(void)
{
	paride_unregister(&k951);
	paride_unregister(&k971);
}

MODULE_LICENSE("GPL");
module_init(kbic_init)
module_exit(kbic_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /* 
        ktti.c        (c) 1998  Grant R. Guenther <grant@torque.net>
                          Under the terms of the GNU General Public License.

	ktti.c is a low-level protocol driver for the KT Technology
	parallel port adapter.  This adapter is used in the "PHd" 
        portable hard-drives.  As far as I can tell, this device
	supports 4-bit mode _only_.  

*/

#define KTTI_VERSION      "1.0"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#define j44(a,b)                (((a>>4)&0x0f)|(b&0xf0))

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int  cont_map[2] = { 0x10, 0x08 };

static void  ktti_write_regr( PIA *pi, int cont, int regr, int val)

{	int r;

	r = regr + cont_map[cont];

	w0(r); w2(0xb); w2(0xa); w2(3); w2(6); 
	w0(val); w2(3); w0(0); w2(6); w2(0xb);
}

static int ktti_read_regr( PIA *pi, int cont, int regr )

{	int  a, b, r;

        r = regr + cont_map[cont];

        w0(r); w2(0xb); w2(0xa); w2(9); w2(0xc); w2(9); 
	a = r1(); w2(0xc);  b = r1(); w2(9); w2(0xc); w2(9);
	return j44(a,b);

}

static void ktti_read_block( PIA *pi, char * buf, int count )

{	int  k, a, b;

	for (k=0;k<count/2;k++) {
		w0(0x10); w2(0xb); w2(0xa); w2(9); w2(0xc); w2(9);
		a = r1(); w2(0xc); b = r1(); w2(9);
		buf[2*k] = j44(a,b);
		a = r1(); w2(0xc); b = r1(); w2(9);
		buf[2*k+1] = j44(a,b);
	}
}

static void ktti_write_block( PIA *pi, char * buf, int count )

{	int k;

	for (k=0;k<count/2;k++) {
		w0(0x10); w2(0xb); w2(0xa); w2(3); w2(6);
		w0(buf[2*k]); w2(3);
		w0(buf[2*k+1]); w2(6);
		w2(0xb);
	}
}

static void ktti_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	w2(0xb); w2(0xa); w0(0); w2(3); w2(6);	
}

static void ktti_disconnect ( PIA *pi )

{       w2(0xb); w2(0xa); w0(0xa0); w2(3); w2(4);
	w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void ktti_log_adapter( PIA *pi, char * scratch, int verbose )

{       printk("%s: ktti %s, KT adapter at 0x%x, delay %d\n",
                pi->device,KTTI_VERSION,pi->port,pi->delay);

}

static struct pi_protocol ktti = {
	.owner		= THIS_MODULE,
	.name		= "ktti",
	.max_mode	= 1,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= ktti_write_regr,
	.read_regr	= ktti_read_regr,
	.write_block	= ktti_write_block,
	.read_block	= ktti_read_block,
	.connect	= ktti_connect,
	.disconnect	= ktti_disconnect,
	.log_adapter	= ktti_log_adapter,
};

static int __init ktti_init(void)
{
	return paride_register(&ktti);
}

static void __exit ktti_exit(void)
{
	paride_unregister(&ktti);
}

MODULE_LICENSE("GPL");
module_init(ktti_init)
module_exit(ktti_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  #!/bin/bash
#
# mkd -- a script to create the device special files for the PARIDE subsystem
#
#  block devices:  	pd (45), pcd (46), pf (47)
#  character devices:	pt (96), pg (97)
#
function mkdev {
  mknod $1 $2 $3 $4 ; chmod 0660 $1 ; chown root:disk $1
}
#
function pd {
  D=$( printf \\$( printf "x%03x" $[ $1 + 97 ] ) )
  mkdev pd$D b 45 $[ $1 * 16 ]
  for P in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
  do mkdev pd$D$P b 45 $[ $1 * 16 + $P ]
  done
}
#
cd /dev
#
for u in 0 1 2 3 ; do pd $u ; done
for u in 0 1 2 3 ; do mkdev pcd$u b 46 $u ; done 
for u in 0 1 2 3 ; do mkdev pf$u  b 47 $u ; done 
for u in 0 1 2 3 ; do mkdev pt$u  c 96 $u ; done 
for u in 0 1 2 3 ; do mkdev npt$u c 96 $[ $u + 128 ] ; done 
for u in 0 1 2 3 ; do mkdev pg$u  c 97 $u ; done
#
# end of mkd

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /* 
	on20.c	(c) 1996-8  Grant R. Guenther <grant@torque.net>
		            Under the terms of the GNU General Public License.

        on20.c is a low-level protocol driver for the
        Onspec 90c20 parallel to IDE adapter. 
*/

/* Changes:

        1.01    GRG 1998.05.06 init_proto, release_proto

*/

#define	ON20_VERSION	"1.01"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#define op(f)	w2(4);w0(f);w2(5);w2(0xd);w2(5);w2(0xd);w2(5);w2(4);
#define vl(v)	w2(4);w0(v);w2(5);w2(7);w2(5);w2(4);

#define j44(a,b)  (((a>>4)&0x0f)|(b&0xf0))

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int on20_read_regr( PIA *pi, int cont, int regr )

{	int h,l, r ;

        r = (regr<<2) + 1 + cont;

        op(1); vl(r); op(0);

	switch (pi->mode)  {

        case 0:  w2(4); w2(6); l = r1();
                 w2(4); w2(6); h = r1();
                 w2(4); w2(6); w2(4); w2(6); w2(4);
		 return j44(l,h);

	case 1:  w2(4); w2(0x26); r = r0(); 
                 w2(4); w2(0x26); w2(4);
		 return r;

	}
	return -1;
}	

static void on20_write_regr( PIA *pi, int cont, int regr, int val )

{	int r;

	r = (regr<<2) + 1 + cont;

	op(1); vl(r); 
	op(0); vl(val); 
	op(0); vl(val);
}

static void on20_connect ( PIA *pi)

{	pi->saved_r0 = r0();
        pi->saved_r2 = r2();

	w2(4);w0(0);w2(0xc);w2(4);w2(6);w2(4);w2(6);w2(4); 
	if (pi->mode) { op(2); vl(8); op(2); vl(9); }
	       else   { op(2); vl(0); op(2); vl(8); }
}

static void on20_disconnect ( PIA *pi )

{	w2(4);w0(7);w2(4);w2(0xc);w2(4);
        w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void on20_read_block( PIA *pi, char * buf, int count )

{	int     k, l, h; 

	op(1); vl(1); op(0);

	for (k=0;k<count;k++) 
	    if (pi->mode) {
		w2(4); w2(0x26); buf[k] = r0();
	    } else {
		w2(6); l = r1(); w2(4);
		w2(6); h = r1(); w2(4);
		buf[k] = j44(l,h);
	    }
	w2(4);
}

static void on20_write_block(  PIA *pi, char * buf, int count )

{	int	k;

	op(1); vl(1); op(0);

	for (k=0;k<count;k++) { w2(5); w0(buf[k]); w2(7); }
	w2(4);
}

static void on20_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[2] = {"4-bit","8-bit"};

        printk("%s: on20 %s, OnSpec 90c20 at 0x%x, ",
                pi->device,ON20_VERSION,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol on20 = {
	.owner		= THIS_MODULE,
	.name		= "on20",
	.max_mode	= 2,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= on20_write_regr,
	.read_regr	= on20_read_regr,
	.write_block	= on20_write_block,
	.read_block	= on20_read_block,
	.connect	= on20_connect,
	.disconnect	= on20_disconnect,
	.log_adapter	= on20_log_adapter,
};

static int __init on20_init(void)
{
	return paride_register(&on20);
}

static void __exit on20_exit(void)
{
	paride_unregister(&on20);
}

MODULE_LICENSE("GPL");
module_init(on20_init)
module_exit(on20_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /* 
        on26.c    (c) 1997-8  Grant R. Guenther <grant@torque.net>
                              Under the terms of the GNU General Public License.

        on26.c is a low-level protocol driver for the 
        OnSpec 90c26 parallel to IDE adapter chip.

*/

/* Changes:

        1.01    GRG 1998.05.06 init_proto, release_proto
	1.02    GRG 1998.09.23 updates for the -E rev chip
	1.03    GRG 1998.12.14 fix for slave drives
	1.04    GRG 1998.12.20 yet another bug fix

*/

#define ON26_VERSION      "1.04"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

/* mode codes:  0  nybble reads, 8-bit writes
                1  8-bit reads and writes
                2  8-bit EPP mode
		3  EPP-16
		4  EPP-32
*/

#define j44(a,b)  (((a>>4)&0x0f)|(b&0xf0))

#define P1	w2(5);w2(0xd);w2(5);w2(0xd);w2(5);w2(4);
#define P2	w2(5);w2(7);w2(5);w2(4);

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int on26_read_regr( PIA *pi, int cont, int regr )

{       int     a, b, r;

	r = (regr<<2) + 1 + cont;

        switch (pi->mode)  {

        case 0: w0(1); P1; w0(r); P2; w0(0); P1; 
		w2(6); a = r1(); w2(4);
		w2(6); b = r1(); w2(4);
		w2(6); w2(4); w2(6); w2(4);
                return j44(a,b);

        case 1: w0(1); P1; w0(r); P2; w0(0); P1;
		w2(0x26); a = r0(); w2(4); w2(0x26); w2(4);
                return a;

	case 2:
	case 3:
        case 4: w3(1); w3(1); w2(5); w4(r); w2(4);
		w3(0); w3(0); w2(0x24); a = r4(); w2(4);
		w2(0x24); (void)r4(); w2(4);
                return a;

        }
        return -1;
}       

static void on26_write_regr( PIA *pi, int cont, int regr, int val )

{       int  r;

        r = (regr<<2) + 1 + cont;

        switch (pi->mode)  {

        case 0:
        case 1: w0(1); P1; w0(r); P2; w0(0); P1;
		w0(val); P2; w0(val); P2;
		break;

	case 2:
	case 3:
        case 4: w3(1); w3(1); w2(5); w4(r); w2(4);
		w3(0); w3(0); 
		w2(5); w4(val); w2(4);
		w2(5); w4(val); w2(4);
                break;
        }
}

#define  CCP(x)  w0(0xfe);w0(0xaa);w0(0x55);w0(0);w0(0xff);\
		 w0(0x87);w0(0x78);w0(x);w2(4);w2(5);w2(4);w0(0xff);

static void on26_connect ( PIA *pi )

{       int	x;

	pi->saved_r0 = r0();
        pi->saved_r2 = r2();

        CCP(0x20);
	x = 8; if (pi->mode) x = 9;

	w0(2); P1; w0(8); P2;
	w0(2); P1; w0(x); P2;
}

static void on26_disconnect ( PIA *pi )

{       if (pi->mode >= 2) { w3(4); w3(4); w3(4); w3(4); }
	              else { w0(4); P1; w0(4); P1; }
	CCP(0x30);
        w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

#define	RESET_WAIT  200

static int on26_test_port( PIA *pi)  /* hard reset */

{       int     i, m, d, x=0, y=0;

        pi->saved_r0 = r0();
        pi->saved_r2 = r2();

        d = pi->delay;
        m = pi->mode;
        pi->delay = 5;
        pi->mode = 0;

        w2(0xc);

        CCP(0x30); CCP(0); 

        w0(0xfe);w0(0xaa);w0(0x55);w0(0);w0(0xff);
        i = ((r1() & 0xf0) << 4); w0(0x87);
        i |= (r1() & 0xf0); w0(0x78);
        w0(0x20);w2(4);w2(5);
        i |= ((r1() & 0xf0) >> 4);
        w2(4);w0(0xff);

        if (i == 0xb5f) {

            w0(2); P1; w0(0);   P2;
            w0(3); P1; w0(0);   P2;
            w0(2); P1; w0(8);   P2; udelay(100);
            w0(2); P1; w0(0xa); P2; udelay(100);
            w0(2); P1; w0(8);   P2; udelay(1000);
            
            on26_write_regr(pi,0,6,0xa0);

            for (i=0;i<RESET_WAIT;i++) {
                on26_write_regr(pi,0,6,0xa0);
                x = on26_read_regr(pi,0,7);
                on26_write_regr(pi,0,6,0xb0);
                y = on26_read_regr(pi,0,7);
                if (!((x&0x80)||(y&0x80))) break;
                mdelay(100);
            }

	    if (i == RESET_WAIT) 
		printk("on26: Device reset failed (%x,%x)\n",x,y);

            w0(4); P1; w0(4); P1;
        }

        CCP(0x30);

        pi->delay = d;
        pi->mode = m;
        w0(pi->saved_r0);
        w2(pi->saved_r2);

        return 5;
}


static void on26_read_block( PIA *pi, char * buf, int count )

{       int     k, a, b;

        switch (pi->mode) {

        case 0: w0(1); P1; w0(1); P2; w0(2); P1; w0(0x18); P2; w0(0); P1;
		udelay(10);
		for (k=0;k<count;k++) {
                        w2(6); a = r1();
                        w2(4); b = r1();
                        buf[k] = j44(a,b);
                }
		w0(2); P1; w0(8); P2; 
                break;

        case 1: w0(1); P1; w0(1); P2; w0(2); P1; w0(0x19); P2; w0(0); P1;
		udelay(10);
                for (k=0;k<count/2;k++) {
                        w2(0x26); buf[2*k] = r0();  
			w2(0x24); buf[2*k+1] = r0();
                }
                w0(2); P1; w0(9); P2;
                break;

        case 2: w3(1); w3(1); w2(5); w4(1); w2(4);
		w3(0); w3(0); w2(0x24);
		udelay(10);
                for (k=0;k<count;k++) buf[k] = r4();
                w2(4);
                break;

        case 3: w3(1); w3(1); w2(5); w4(1); w2(4);
                w3(0); w3(0); w2(0x24);
                udelay(10);
                for (k=0;k<count/2;k++) ((u16 *)buf)[k] = r4w();
                w2(4);
                break;

        case 4: w3(1); w3(1); w2(5); w4(1); w2(4);
                w3(0); w3(0); w2(0x24);
                udelay(10);
                for (k=0;k<count/4;k++) ((u32 *)buf)[k] = r4l();
                w2(4);
                break;

        }
}

static void on26_write_block( PIA *pi, char * buf, int count )

{       int	k;

        switch (pi->mode) {

        case 0: 
        case 1: w0(1); P1; w0(1); P2; 
		w0(2); P1; w0(0x18+pi->mode); P2; w0(0); P1;
		udelay(10);
		for (k=0;k<count/2;k++) {
                        w2(5); w0(buf[2*k]); 
			w2(7); w0(buf[2*k+1]);
                }
                w2(5); w2(4);
		w0(2); P1; w0(8+pi->mode); P2;
                break;

        case 2: w3(1); w3(1); w2(5); w4(1); w2(4);
		w3(0); w3(0); w2(0xc5);
		udelay(10);
                for (k=0;k<count;k++) w4(buf[k]);
		w2(0xc4);
                break;

        case 3: w3(1); w3(1); w2(5); w4(1); w2(4);
                w3(0); w3(0); w2(0xc5);
                udelay(10);
                for (k=0;k<count/2;k++) w4w(((u16 *)buf)[k]);
                w2(0xc4);
                break;

        case 4: w3(1); w3(1); w2(5); w4(1); w2(4);
                w3(0); w3(0); w2(0xc5);
                udelay(10);
                for (k=0;k<count/4;k++) w4l(((u32 *)buf)[k]);
                w2(0xc4);
                break;

        }

}

static void on26_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[5] = {"4-bit","8-bit","EPP-8",
				   "EPP-16","EPP-32"};

        printk("%s: on26 %s, OnSpec 90c26 at 0x%x, ",
                pi->device,ON26_VERSION,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol on26 = {
	.owner		= THIS_MODULE,
	.name		= "on26",
	.max_mode	= 5,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= on26_write_regr,
	.read_regr	= on26_read_regr,
	.write_block	= on26_write_block,
	.read_block	= on26_read_block,
	.connect	= on26_connect,
	.disconnect	= on26_disconnect,
	.test_port	= on26_test_port,
	.log_adapter	= on26_log_adapter,
};

static int __init on26_init(void)
{
	return paride_register(&on26);
}

static void __exit on26_exit(void)
{
	paride_unregister(&on26);
}

MODULE_LICENSE("GPL");
module_init(on26_init)
module_exit(on26_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /* 
        paride.c  (c) 1997-8  Grant R. Guenther <grant@torque.net>
                              Under the terms of the GNU General Public License.

	This is the base module for the family of device drivers
        that support parallel port IDE devices.  

*/

/* Changes:

	1.01	GRG 1998.05.03	Use spinlocks
	1.02	GRG 1998.05.05  init_proto, release_proto, ktti
	1.03	GRG 1998.08.15  eliminate compiler warning
	1.04    GRG 1998.11.28  added support for FRIQ 
	1.05    TMW 2000.06.06  use parport_find_number instead of
				parport_enumerate
	1.06    TMW 2001.03.26  more sane parport-or-not resource management
*/

#define PI_VERSION      "1.06"

#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>	/* TASK_* */
#include <linux/parport.h>
#include <linux/slab.h>

#include "paride.h"

MODULE_LICENSE("GPL");

#define MAX_PROTOS	32

static struct pi_protocol *protocols[MAX_PROTOS];

static DEFINE_SPINLOCK(pi_spinlock);

void pi_write_regr(PIA * pi, int cont, int regr, int val)
{
	pi->proto->write_regr(pi, cont, regr, val);
}

EXPORT_SYMBOL(pi_write_regr);

int pi_read_regr(PIA * pi, int cont, int regr)
{
	return pi->proto->read_regr(pi, cont, regr);
}

EXPORT_SYMBOL(pi_read_regr);

void pi_write_block(PIA * pi, char *buf, int count)
{
	pi->proto->write_block(pi, buf, count);
}

EXPORT_SYMBOL(pi_write_block);

void pi_read_block(PIA * pi, char *buf, int count)
{
	pi->proto->read_block(pi, buf, count);
}

EXPORT_SYMBOL(pi_read_block);

static void pi_wake_up(void *p)
{
	PIA *pi = (PIA *) p;
	unsigned long flags;
	void (*cont) (void) = NULL;

	spin_lock_irqsave(&pi_spinlock, flags);

	if (pi->claim_cont && !parport_claim(pi->pardev)) {
		cont = pi->claim_cont;
		pi->claim_cont = NULL;
		pi->claimed = 1;
	}

	spin_unlock_irqrestore(&pi_spinlock, flags);

	wake_up(&(pi->parq));

	if (cont)
		cont();
}

int pi_schedule_claimed(PIA * pi, void (*cont) (void))
{
	unsigned long flags;

	spin_lock_irqsave(&pi_spinlock, flags);
	if (pi->pardev && parport_claim(pi->pardev)) {
		pi->claim_cont = cont;
		spin_unlock_irqrestore(&pi_spinlock, flags);
		return 0;
	}
	pi->claimed = 1;
	spin_unlock_irqrestore(&pi_spinlock, flags);
	return 1;
}
EXPORT_SYMBOL(pi_schedule_claimed);

void pi_do_claimed(PIA * pi, void (*cont) (void))
{
	if (pi_schedule_claimed(pi, cont))
		cont();
}

EXPORT_SYMBOL(pi_do_claimed);

static void pi_claim(PIA * pi)
{
	if (pi->claimed)
		return;
	pi->claimed = 1;
	if (pi->pardev)
		wait_event(pi->parq,
			   !parport_claim((struct pardevice *) pi->pardev));
}

static void pi_unclaim(PIA * pi)
{
	pi->claimed = 0;
	if (pi->pardev)
		parport_release((struct pardevice *) (pi->pardev));
}

void pi_connect(PIA * pi)
{
	pi_claim(pi);
	pi->proto->connect(pi);
}

EXPORT_SYMBOL(pi_connect);

void pi_disconnect(PIA * pi)
{
	pi->proto->disconnect(pi);
	pi_unclaim(pi);
}

EXPORT_SYMBOL(pi_disconnect);

static void pi_unregister_parport(PIA * pi)
{
	if (pi->pardev) {
		parport_unregister_device((struct pardevice *) (pi->pardev));
		pi->pardev = NULL;
	}
}

void pi_release(PIA * pi)
{
	pi_unregister_parport(pi);
	if (pi->proto->release_proto)
		pi->proto->release_proto(pi);
	module_put(pi->proto->owner);
}

EXPORT_SYMBOL(pi_release);

static int default_test_proto(PIA * pi, char *scratch, int verbose)
{
	int j, k;
	int e[2] = { 0, 0 };

	pi->proto->connect(pi);

	for (j = 0; j < 2; j++) {
		pi_write_regr(pi, 0, 6, 0xa0 + j * 0x10);
		for (k = 0; k < 256; k++) {
			pi_write_regr(pi, 0, 2, k ^ 0xaa);
			pi_write_regr(pi, 0, 3, k ^ 0x55);
			if (pi_read_regr(pi, 0, 2) != (k ^ 0xaa))
				e[j]++;
		}
	}
	pi->proto->disconnect(pi);

	if (verbose)
		printk("%s: %s: port 0x%x, mode  %d, test=(%d,%d)\n",
		       pi->device, pi->proto->name, pi->port,
		       pi->mode, e[0], e[1]);

	return (e[0] && e[1]);	/* not here if both > 0 */
}

static int pi_test_proto(PIA * pi, char *scratch, int verbose)
{
	int res;

	pi_claim(pi);
	if (pi->proto->test_proto)
		res = pi->proto->test_proto(pi, scratch, verbose);
	else
		res = default_test_proto(pi, scratch, verbose);
	pi_unclaim(pi);

	return res;
}

int paride_register(PIP * pr)
{
	int k;

	for (k = 0; k < MAX_PROTOS; k++)
		if (protocols[k] && !strcmp(pr->name, protocols[k]->name)) {
			printk("paride: %s protocol already registered\n",
			       pr->name);
			return -1;
		}
	k = 0;
	while ((k < MAX_PROTOS) && (protocols[k]))
		k++;
	if (k == MAX_PROTOS) {
		printk("paride: protocol table full\n");
		return -1;
	}
	protocols[k] = pr;
	pr->index = k;
	printk("paride: %s registered as protocol %d\n", pr->name, k);
	return 0;
}

EXPORT_SYMBOL(paride_register);

void paride_unregister(PIP * pr)
{
	if (!pr)
		return;
	if (protocols[pr->index] != pr) {
		printk("paride: %s not registered\n", pr->name);
		return;
	}
	protocols[pr->index] = NULL;
}

EXPORT_SYMBOL(paride_unregister);

static int pi_register_parport(PIA *pi, int verbose, int unit)
{
	struct parport *port;
	struct pardev_cb par_cb;

	port = parport_find_base(pi->port);
	if (!port)
		return 0;
	memset(&par_cb, 0, sizeof(par_cb));
	par_cb.wakeup = pi_wake_up;
	par_cb.private = (void *)pi;
	pi->pardev = parport_register_dev_model(port, pi->device, &par_cb,
						unit);
	parport_put_port(port);
	if (!pi->pardev)
		return 0;

	init_waitqueue_head(&pi->parq);

	if (verbose)
		printk("%s: 0x%x is %s\n", pi->device, pi->port, port->name);

	pi->parname = (char *) port->name;

	return 1;
}

static int pi_probe_mode(PIA * pi, int max, char *scratch, int verbose)
{
	int best, range;

	if (pi->mode != -1) {
		if (pi->mode >= max)
			return 0;
		range = 3;
		if (pi->mode >= pi->proto->epp_first)
			range = 8;
		if ((range == 8) && (pi->port % 8))
			return 0;
		pi->reserved = range;
		return (!pi_test_proto(pi, scratch, verbose));
	}
	best = -1;
	for (pi->mode = 0; pi->mode < max; pi->mode++) {
		range = 3;
		if (pi->mode >= pi->proto->epp_first)
			range = 8;
		if ((range == 8) && (pi->port % 8))
			break;
		pi->reserved = range;
		if (!pi_test_proto(pi, scratch, verbose))
			best = pi->mode;
	}
	pi->mode = best;
	return (best > -1);
}

static int pi_probe_unit(PIA * pi, int unit, char *scratch, int verbose)
{
	int max, s, e;

	s = unit;
	e = s + 1;

	if (s == -1) {
		s = 0;
		e = pi->proto->max_units;
	}

	if (!pi_register_parport(pi, verbose, s))
		return 0;

	if (pi->proto->test_port) {
		pi_claim(pi);
		max = pi->proto->test_port(pi);
		pi_unclaim(pi);
	} else
		max = pi->proto->max_mode;

	if (pi->proto->probe_unit) {
		pi_claim(pi);
		for (pi->unit = s; pi->unit < e; pi->unit++)
			if (pi->proto->probe_unit(pi)) {
				pi_unclaim(pi);
				if (pi_probe_mode(pi, max, scratch, verbose))
					return 1;
				pi_unregister_parport(pi);
				return 0;
			}
		pi_unclaim(pi);
		pi_unregister_parport(pi);
		return 0;
	}

	if (!pi_probe_mode(pi, max, scratch, verbose)) {
		pi_unregister_parport(pi);
		return 0;
	}
	return 1;

}

int pi_init(PIA * pi, int autoprobe, int port, int mode,
	int unit, int protocol, int delay, char *scratch,
	int devtype, int verbose, char *device)
{
	int p, k, s, e;
	int lpts[7] = { 0x3bc, 0x378, 0x278, 0x268, 0x27c, 0x26c, 0 };

	s = protocol;
	e = s + 1;

	if (!protocols[0])
		request_module("paride_protocol");

	if (autoprobe) {
		s = 0;
		e = MAX_PROTOS;
	} else if ((s < 0) || (s >= MAX_PROTOS) || (port <= 0) ||
		   (!protocols[s]) || (unit < 0) ||
		   (unit >= protocols[s]->max_units)) {
		printk("%s: Invalid parameters\n", device);
		return 0;
	}

	for (p = s; p < e; p++) {
		struct pi_protocol *proto = protocols[p];
		if (!proto)
			continue;
		/* still racy */
		if (!try_module_get(proto->owner))
			continue;
		pi->proto = proto;
		pi->private = 0;
		if (proto->init_proto && proto->init_proto(pi) < 0) {
			pi->proto = NULL;
			module_put(proto->owner);
			continue;
		}
		if (delay == -1)
			pi->delay = pi->proto->default_delay;
		else
			pi->delay = delay;
		pi->devtype = devtype;
		pi->device = device;

		pi->parname = NULL;
		pi->pardev = NULL;
		init_waitqueue_head(&pi->parq);
		pi->claimed = 0;
		pi->claim_cont = NULL;

		pi->mode = mode;
		if (port != -1) {
			pi->port = port;
			if (pi_probe_unit(pi, unit, scratch, verbose))
				break;
			pi->port = 0;
		} else {
			k = 0;
			while ((pi->port = lpts[k++]))
				if (pi_probe_unit
				    (pi, unit, scratch, verbose))
					break;
			if (pi->port)
				break;
		}
		if (pi->proto->release_proto)
			pi->proto->release_proto(pi);
		module_put(proto->owner);
	}

	if (!pi->port) {
		if (autoprobe)
			printk("%s: Autoprobe failed\n", device);
		else
			printk("%s: Adapter not found\n", device);
		return 0;
	}

	if (pi->parname)
		printk("%s: Sharing %s at 0x%x\n", pi->device,
		       pi->parname, pi->port);

	pi->proto->log_adapter(pi, scratch, verbose);

	return 1;
}

EXPORT_SYMBOL(pi_init);

static int pi_probe(struct pardevice *par_dev)
{
	struct device_driver *drv = par_dev->dev.driver;
	int len = strlen(drv->name);

	if (strncmp(par_dev->name, drv->name, len))
		return -ENODEV;

	return 0;
}

void *pi_register_driver(char *name)
{
	struct parport_driver *parp_drv;
	int ret;

	parp_drv = kzalloc(sizeof(*parp_drv), GFP_KERNEL);
	if (!parp_drv)
		return NULL;

	parp_drv->name = name;
	parp_drv->probe = pi_probe;
	parp_drv->devmodel = true;

	ret = parport_register_driver(parp_drv);
	if (ret) {
		kfree(parp_drv);
		return NULL;
	}
	return (void *)parp_drv;
}
EXPORT_SYMBOL(pi_register_driver);

void pi_unregister_driver(void *_drv)
{
	struct parport_driver *drv = _drv;

	parport_unregister_driver(drv);
	kfree(drv);
}
EXPORT_SYMBOL(pi_unregister_driver);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #ifndef __DRIVERS_PARIDE_H__
#define __DRIVERS_PARIDE_H__

/* 
	paride.h	(c) 1997-8  Grant R. Guenther <grant@torque.net>
   		                    Under the terms of the GPL.

   This file defines the interface between the high-level parallel
   IDE device drivers (pd, pf, pcd, pt) and the adapter chips.

*/

/* Changes:

	1.01	GRG 1998.05.05	init_proto, release_proto
*/

#define PARIDE_H_VERSION 	"1.01"

/* Some adapters need to know what kind of device they are in

   Values for devtype:
*/

#define	PI_PD	0	/* IDE disk */
#define PI_PCD	1	/* ATAPI CDrom */
#define PI_PF   2	/* ATAPI disk */
#define PI_PT	3	/* ATAPI tape */
#define PI_PG   4       /* ATAPI generic */

/* The paride module contains no state, instead the drivers allocate
   a pi_adapter data structure and pass it to paride in every operation.

*/

struct pi_adapter  {

	struct pi_protocol *proto;   /* adapter protocol */
	int	port;		     /* base address of parallel port */
	int	mode;		     /* transfer mode in use */
	int     delay;		     /* adapter delay setting */
	int	devtype;	     /* device type: PI_PD etc. */
	char    *device;	     /* name of driver */
	int     unit;		     /* unit number for chained adapters */
	int	saved_r0;	     /* saved port state */
	int	saved_r2;	     /* saved port state */
	int	reserved;	     /* number of ports reserved */
	unsigned long	private;     /* for protocol module */

	wait_queue_head_t parq;     /* semaphore for parport sharing */
	void	*pardev;	     /* pointer to pardevice */
	char	*parname;	     /* parport name */
	int	claimed;	     /* parport has already been claimed */
	void (*claim_cont)(void);    /* continuation for parport wait */
};

typedef struct pi_adapter PIA;

/* functions exported by paride to the high level drivers */

extern int pi_init(PIA *pi, 
	int autoprobe,		/* 1 to autoprobe */
	int port, 		/* base port address */
	int mode, 		/* -1 for autoprobe */
	int unit,		/* unit number, if supported */
	int protocol, 		/* protocol to use */
	int delay, 		/* -1 to use adapter specific default */
	char * scratch, 	/* address of 512 byte buffer */
	int devtype,		/* device type: PI_PD, PI_PCD, etc ... */
	int verbose,		/* log verbose data while probing */
	char *device		/* name of the driver */
	);			/* returns 0 on failure, 1 on success */

extern void pi_release(PIA *pi);

/* registers are addressed as (cont,regr)

       	cont: 0 for command register file, 1 for control register(s)
	regr: 0-7 for register number.

*/

extern void pi_write_regr(PIA *pi, int cont, int regr, int val);

extern int pi_read_regr(PIA *pi, int cont, int regr);

extern void pi_write_block(PIA *pi, char * buf, int count);

extern void pi_read_block(PIA *pi, char * buf, int count);

extern void pi_connect(PIA *pi);

extern void pi_disconnect(PIA *pi);

extern void pi_do_claimed(PIA *pi, void (*cont)(void));
extern int pi_schedule_claimed(PIA *pi, void (*cont)(void));

/* macros and functions exported to the protocol modules */

#define delay_p			(pi->delay?udelay(pi->delay):(void)0)
#define out_p(offs,byte)	outb(byte,pi->port+offs); delay_p;
#define in_p(offs)		(delay_p,inb(pi->port+offs))

#define w0(byte)                {out_p(0,byte);}
#define r0()                    (in_p(0) & 0xff)
#define w1(byte)                {out_p(1,byte);}
#define r1()                    (in_p(1) & 0xff)
#define w2(byte)                {out_p(2,byte);}
#define r2()                    (in_p(2) & 0xff)
#define w3(byte)                {out_p(3,byte);}
#define w4(byte)                {out_p(4,byte);}
#define r4()                    (in_p(4) & 0xff)
#define w4w(data)     		{outw(data,pi->port+4); delay_p;}
#define w4l(data)     		{outl(data,pi->port+4); delay_p;}
#define r4w()         		(delay_p,inw(pi->port+4)&0xffff)
#define r4l()         		(delay_p,inl(pi->port+4)&0xffffffff)

static inline u16 pi_swab16( char *b, int k)

{ 	union { u16 u; char t[2]; } r;

	r.t[0]=b[2*k+1]; r.t[1]=b[2*k];
        return r.u;
}

static inline u32 pi_swab32( char *b, int k)

{ 	union { u32 u; char f[4]; } r;

	r.f[0]=b[4*k+1]; r.f[1]=b[4*k];
	r.f[2]=b[4*k+3]; r.f[3]=b[4*k+2];
        return r.u;
}

struct pi_protocol {

	char	name[8];	/* name for this protocol */
	int	index;		/* index into protocol table */

	int	max_mode;	/* max mode number */
	int	epp_first;	/* modes >= this use 8 ports */
	
	int	default_delay;  /* delay parameter if not specified */
	int	max_units;	/* max chained units probed for */

	void (*write_regr)(PIA *,int,int,int);
	int  (*read_regr)(PIA *,int,int);
	void (*write_block)(PIA *,char *,int);
	void (*read_block)(PIA *,char *,int);

	void (*connect)(PIA *);
	void (*disconnect)(PIA *);
	
	int  (*test_port)(PIA *);
	int  (*probe_unit)(PIA *);
	int  (*test_proto)(PIA *,char *,int);
	void (*log_adapter)(PIA *,char *,int);
	
	int (*init_proto)(PIA *);
	void (*release_proto)(PIA *);
	struct module *owner;
};

typedef struct pi_protocol PIP;

extern int paride_register( PIP * );
extern void paride_unregister ( PIP * );
void *pi_register_driver(char *);
void pi_unregister_driver(void *);

#endif /* __DRIVERS_PARIDE_H__ */
/* end of paride.h */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /* 
	pcd.c	(c) 1997-8  Grant R. Guenther <grant@torque.net>
		            Under the terms of the GNU General Public License.

	This is a high-level driver for parallel port ATAPI CD-ROM
        drives based on chips supported by the paride module.

        By default, the driver will autoprobe for a single parallel
        port ATAPI CD-ROM drive, but if their individual parameters are
        specified, the driver can handle up to 4 drives.

        The behaviour of the pcd driver can be altered by setting
        some parameters from the insmod command line.  The following
        parameters are adjustable:

            drive0      These four arguments can be arrays of       
            drive1      1-6 integers as follows:
            drive2
            drive3      <prt>,<pro>,<uni>,<mod>,<slv>,<dly>

                        Where,

                <prt>   is the base of the parallel port address for
                        the corresponding drive.  (required)

                <pro>   is the protocol number for the adapter that
                        supports this drive.  These numbers are
                        logged by 'paride' when the protocol modules
                        are initialised.  (0 if not given)

                <uni>   for those adapters that support chained
                        devices, this is the unit selector for the
                        chain of devices on the given port.  It should
                        be zero for devices that don't support chaining.
                        (0 if not given)

                <mod>   this can be -1 to choose the best mode, or one
                        of the mode numbers supported by the adapter.
                        (-1 if not given)

		<slv>   ATAPI CD-ROMs can be jumpered to master or slave.
			Set this to 0 to choose the master drive, 1 to
                        choose the slave, -1 (the default) to choose the
			first drive found.

                <dly>   some parallel ports require the driver to 
                        go more slowly.  -1 sets a default value that
                        should work with the chosen protocol.  Otherwise,
                        set this to a small integer, the larger it is
                        the slower the port i/o.  In some cases, setting
                        this to zero will speed up the device. (default -1)
                        
            major       You may use this parameter to overide the
                        default major number (46) that this driver
                        will use.  Be sure to change the device
                        name as well.

            name        This parameter is a character string that
                        contains the name the kernel will use for this
                        device (in /proc output, for instance).
                        (default "pcd")

            verbose     This parameter controls the amount of logging
                        that the driver will do.  Set it to 0 for
                        normal operation, 1 to see autoprobe progress
                        messages, or 2 to see additional debugging
                        output.  (default 0)
  
            nice        This parameter controls the driver's use of
                        idle CPU time, at the expense of some speed.
 
	If this driver is built into the kernel, you can use the
        following kernel command line parameters, with the same values
        as the corresponding module parameters listed above:

	    pcd.drive0
	    pcd.drive1
	    pcd.drive2
	    pcd.drive3
	    pcd.nice

        In addition, you can use the parameter pcd.disable to disable
        the driver entirely.

*/

/* Changes:

	1.01	GRG 1998.01.24	Added test unit ready support
	1.02    GRG 1998.05.06  Changes to pcd_completion, ready_wait,
				and loosen interpretation of ATAPI
			        standard for clearing error status.
				Use spinlocks. Eliminate sti().
	1.03    GRG 1998.06.16  Eliminated an Ugh
	1.04	GRG 1998.08.15  Added extra debugging, improvements to
				pcd_completion, use HZ in loop timing
	1.05	GRG 1998.08.16	Conformed to "Uniform CD-ROM" standard
	1.06    GRG 1998.08.19  Added audio ioctl support
	1.07    GRG 1998.09.24  Increased reset timeout, added jumbo support

*/

#define	PCD_VERSION	"1.07"
#define PCD_MAJOR	46
#define PCD_NAME	"pcd"
#define PCD_UNITS	4

/* Here are things one can override from the insmod command.
   Most are autoprobed by paride unless set here.  Verbose is off
   by default.

*/

static int verbose = 0;
static int major = PCD_MAJOR;
static char *name = PCD_NAME;
static int nice = 0;
static int disable = 0;

static int drive0[6] = { 0, 0, 0, -1, -1, -1 };
static int drive1[6] = { 0, 0, 0, -1, -1, -1 };
static int drive2[6] = { 0, 0, 0, -1, -1, -1 };
static int drive3[6] = { 0, 0, 0, -1, -1, -1 };

static int (*drives[4])[6] = {&drive0, &drive1, &drive2, &drive3};
static int pcd_drive_count;

enum {D_PRT, D_PRO, D_UNI, D_MOD, D_SLV, D_DLY};

/* end of parameters */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/cdrom.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

static DEFINE_MUTEX(pcd_mutex);
static DEFINE_SPINLOCK(pcd_lock);

module_param(verbose, int, 0644);
module_param(major, int, 0);
module_param(name, charp, 0);
module_param(nice, int, 0);
module_param_array(drive0, int, NULL, 0);
module_param_array(drive1, int, NULL, 0);
module_param_array(drive2, int, NULL, 0);
module_param_array(drive3, int, NULL, 0);

#include "paride.h"
#include "pseudo.h"

#define PCD_RETRIES	     5
#define PCD_TMO		   800	/* timeout in jiffies */
#define PCD_DELAY           50	/* spin delay in uS */
#define PCD_READY_TMO	    20	/* in seconds */
#define PCD_RESET_TMO	   100	/* in tenths of a second */

#define PCD_SPIN	(1000000*PCD_TMO)/(HZ*PCD_DELAY)

#define IDE_ERR		0x01
#define IDE_DRQ         0x08
#define IDE_READY       0x40
#define IDE_BUSY        0x80

static int pcd_open(struct cdrom_device_info *cdi, int purpose);
static void pcd_release(struct cdrom_device_info *cdi);
static int pcd_drive_status(struct cdrom_device_info *cdi, int slot_nr);
static unsigned int pcd_check_events(struct cdrom_device_info *cdi,
				     unsigned int clearing, int slot_nr);
static int pcd_tray_move(struct cdrom_device_info *cdi, int position);
static int pcd_lock_door(struct cdrom_device_info *cdi, int lock);
static int pcd_drive_reset(struct cdrom_device_info *cdi);
static int pcd_get_mcn(struct cdrom_device_info *cdi, struct cdrom_mcn *mcn);
static int pcd_audio_ioctl(struct cdrom_device_info *cdi,
			   unsigned int cmd, void *arg);
static int pcd_packet(struct cdrom_device_info *cdi,
		      struct packet_command *cgc);

static int pcd_detect(void);
static void pcd_probe_capabilities(void);
static void do_pcd_read_drq(void);
static void do_pcd_request(struct request_queue * q);
static void do_pcd_read(void);

struct pcd_unit {
	struct pi_adapter pia;	/* interface to paride layer */
	struct pi_adapter *pi;
	int drive;		/* master/slave */
	int last_sense;		/* result of last request sense */
	int changed;		/* media change seen */
	int present;		/* does this unit exist ? */
	char *name;		/* pcd0, pcd1, etc */
	struct cdrom_device_info info;	/* uniform cdrom interface */
	struct gendisk *disk;
};

static struct pcd_unit pcd[PCD_UNITS];

static char pcd_scratch[64];
static char pcd_buffer[2048];	/* raw block buffer */
static int pcd_bufblk = -1;	/* block in buffer, in CD units,
				   -1 for nothing there. See also
				   pd_unit.
				 */

/* the variables below are used mainly in the I/O request engine, which
   processes only one request at a time.
*/

static struct pcd_unit *pcd_current; /* current request's drive */
static struct request *pcd_req;
static int pcd_retries;		/* retries on current request */
static int pcd_busy;		/* request being processed ? */
static int pcd_sector;		/* address of next requested sector */
static int pcd_count;		/* number of blocks still to do */
static char *pcd_buf;		/* buffer for request in progress */
static void *par_drv;		/* reference of parport driver */

/* kernel glue structures */

static int pcd_block_open(struct block_device *bdev, fmode_t mode)
{
	struct pcd_unit *cd = bdev->bd_disk->private_data;
	int ret;

	check_disk_change(bdev);

	mutex_lock(&pcd_mutex);
	ret = cdrom_open(&cd->info, bdev, mode);
	mutex_unlock(&pcd_mutex);

	return ret;
}

static void pcd_block_release(struct gendisk *disk, fmode_t mode)
{
	struct pcd_unit *cd = disk->private_data;
	mutex_lock(&pcd_mutex);
	cdrom_release(&cd->info, mode);
	mutex_unlock(&pcd_mutex);
}

static int pcd_block_ioctl(struct block_device *bdev, fmode_t mode,
				unsigned cmd, unsigned long arg)
{
	struct pcd_unit *cd = bdev->bd_disk->private_data;
	int ret;

	mutex_lock(&pcd_mutex);
	ret = cdrom_ioctl(&cd->info, bdev, mode, cmd, arg);
	mutex_unlock(&pcd_mutex);

	return ret;
}

static unsigned int pcd_block_check_events(struct gendisk *disk,
					   unsigned int clearing)
{
	struct pcd_unit *cd = disk->private_data;
	return cdrom_check_events(&cd->info, clearing);
}

static const struct block_device_operations pcd_bdops = {
	.owner		= THIS_MODULE,
	.open		= pcd_block_open,
	.release	= pcd_block_release,
	.ioctl		= pcd_block_ioctl,
	.check_events	= pcd_block_check_events,
};

static struct cdrom_device_ops pcd_dops = {
	.open		= pcd_open,
	.release	= pcd_release,
	.drive_status	= pcd_drive_status,
	.check_events	= pcd_check_events,
	.tray_move	= pcd_tray_move,
	.lock_door	= pcd_lock_door,
	.get_mcn	= pcd_get_mcn,
	.reset		= pcd_drive_reset,
	.audio_ioctl	= pcd_audio_ioctl,
	.generic_packet	= pcd_packet,
	.capability	= CDC_CLOSE_TRAY | CDC_OPEN_TRAY | CDC_LOCK |
			  CDC_MCN | CDC_MEDIA_CHANGED | CDC_RESET |
			  CDC_PLAY_AUDIO | CDC_GENERIC_PACKET | CDC_CD_R |
			  CDC_CD_RW,
};

static void pcd_init_units(void)
{
	struct pcd_unit *cd;
	int unit;

	pcd_drive_count = 0;
	for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++) {
		struct gendisk *disk = alloc_disk(1);
		if (!disk)
			continue;
		cd->disk = disk;
		cd->pi = &cd->pia;
		cd->present = 0;
		cd->last_sense = 0;
		cd->changed = 1;
		cd->drive = (*drives[unit])[D_SLV];
		if ((*drives[unit])[D_PRT])
			pcd_drive_count++;

		cd->name = &cd->info.name[0];
		snprintf(cd->name, sizeof(cd->info.name), "%s%d", name, unit);
		cd->info.ops = &pcd_dops;
		cd->info.handle = cd;
		cd->info.speed = 0;
		cd->info.capacity = 1;
		cd->info.mask = 0;
		disk->major = major;
		disk->first_minor = unit;
		strcpy(disk->disk_name, cd->name);	/* umm... */
		disk->fops = &pcd_bdops;
		disk->flags = GENHD_FL_BLOCK_EVENTS_ON_EXCL_WRITE;
	}
}

static int pcd_open(struct cdrom_device_info *cdi, int purpose)
{
	struct pcd_unit *cd = cdi->handle;
	if (!cd->present)
		return -ENODEV;
	return 0;
}

static void pcd_release(struct cdrom_device_info *cdi)
{
}

static inline int status_reg(struct pcd_unit *cd)
{
	return pi_read_regr(cd->pi, 1, 6);
}

static inline int read_reg(struct pcd_unit *cd, int reg)
{
	return pi_read_regr(cd->pi, 0, reg);
}

static inline void write_reg(struct pcd_unit *cd, int reg, int val)
{
	pi_write_regr(cd->pi, 0, reg, val);
}

static int pcd_wait(struct pcd_unit *cd, int go, int stop, char *fun, char *msg)
{
	int j, r, e, s, p;

	j = 0;
	while ((((r = status_reg(cd)) & go) || (stop && (!(r & stop))))
	       && (j++ < PCD_SPIN))
		udelay(PCD_DELAY);

	if ((r & (IDE_ERR & stop)) || (j > PCD_SPIN)) {
		s = read_reg(cd, 7);
		e = read_reg(cd, 1);
		p = read_reg(cd, 2);
		if (j > PCD_SPIN)
			e |= 0x100;
		if (fun)
			printk("%s: %s %s: alt=0x%x stat=0x%x err=0x%x"
			       " loop=%d phase=%d\n",
			       cd->name, fun, msg, r, s, e, j, p);
		return (s << 8) + r;
	}
	return 0;
}

static int pcd_command(struct pcd_unit *cd, char *cmd, int dlen, char *fun)
{
	pi_connect(cd->pi);

	write_reg(cd, 6, 0xa0 + 0x10 * cd->drive);

	if (pcd_wait(cd, IDE_BUSY | IDE_DRQ, 0, fun, "before command")) {
		pi_disconnect(cd->pi);
		return -1;
	}

	write_reg(cd, 4, dlen % 256);
	write_reg(cd, 5, dlen / 256);
	write_reg(cd, 7, 0xa0);	/* ATAPI packet command */

	if (pcd_wait(cd, IDE_BUSY, IDE_DRQ, fun, "command DRQ")) {
		pi_disconnect(cd->pi);
		return -1;
	}

	if (read_reg(cd, 2) != 1) {
		printk("%s: %s: command phase error\n", cd->name, fun);
		pi_disconnect(cd->pi);
		return -1;
	}

	pi_write_block(cd->pi, cmd, 12);

	return 0;
}

static int pcd_completion(struct pcd_unit *cd, char *buf, char *fun)
{
	int r, d, p, n, k, j;

	r = -1;
	k = 0;
	j = 0;

	if (!pcd_wait(cd, IDE_BUSY, IDE_DRQ | IDE_READY | IDE_ERR,
		      fun, "completion")) {
		r = 0;
		while (read_reg(cd, 7) & IDE_DRQ) {
			d = read_reg(cd, 4) + 256 * read_reg(cd, 5);
			n = (d + 3) & 0xfffc;
			p = read_reg(cd, 2) & 3;

			if ((p == 2) && (n > 0) && (j == 0)) {
				pi_read_block(cd->pi, buf, n);
				if (verbose > 1)
					printk("%s: %s: Read %d bytes\n",
					       cd->name, fun, n);
				r = 0;
				j++;
			} else {
				if (verbose > 1)
					printk
					    ("%s: %s: Unexpected phase %d, d=%d, k=%d\n",
					     cd->name, fun, p, d, k);
				if (verbose < 2)
					printk_once(
					    "%s: WARNING: ATAPI phase errors\n",
					    cd->name);
				mdelay(1);
			}
			if (k++ > PCD_TMO) {
				printk("%s: Stuck DRQ\n", cd->name);
				break;
			}
			if (pcd_wait
			    (cd, IDE_BUSY, IDE_DRQ | IDE_READY | IDE_ERR, fun,
			     "completion")) {
				r = -1;
				break;
			}
		}
	}

	pi_disconnect(cd->pi);

	return r;
}

static void pcd_req_sense(struct pcd_unit *cd, char *fun)
{
	char rs_cmd[12] = { 0x03, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0 };
	char buf[16];
	int r, c;

	r = pcd_command(cd, rs_cmd, 16, "Request sense");
	mdelay(1);
	if (!r)
		pcd_completion(cd, buf, "Request sense");

	cd->last_sense = -1;
	c = 2;
	if (!r) {
		if (fun)
			printk("%s: %s: Sense key: %x, ASC: %x, ASQ: %x\n",
			       cd->name, fun, buf[2] & 0xf, buf[12], buf[13]);
		c = buf[2] & 0xf;
		cd->last_sense =
		    c | ((buf[12] & 0xff) << 8) | ((buf[13] & 0xff) << 16);
	}
	if ((c == 2) || (c == 6))
		cd->changed = 1;
}

static int pcd_atapi(struct pcd_unit *cd, char *cmd, int dlen, char *buf, char *fun)
{
	int r;

	r = pcd_command(cd, cmd, dlen, fun);
	mdelay(1);
	if (!r)
		r = pcd_completion(cd, buf, fun);
	if (r)
		pcd_req_sense(cd, fun);

	return r;
}

static int pcd_packet(struct cdrom_device_info *cdi, struct packet_command *cgc)
{
	return pcd_atapi(cdi->handle, cgc->cmd, cgc->buflen, cgc->buffer,
			 "generic packet");
}

#define DBMSG(msg)	((verbose>1)?(msg):NULL)

static unsigned int pcd_check_events(struct cdrom_device_info *cdi,
				     unsigned int clearing, int slot_nr)
{
	struct pcd_unit *cd = cdi->handle;
	int res = cd->changed;
	if (res)
		cd->changed = 0;
	return res ? DISK_EVENT_MEDIA_CHANGE : 0;
}

static int pcd_lock_door(struct cdrom_device_info *cdi, int lock)
{
	char un_cmd[12] = { 0x1e, 0, 0, 0, lock, 0, 0, 0, 0, 0, 0, 0 };

	return pcd_atapi(cdi->handle, un_cmd, 0, pcd_scratch,
			 lock ? "lock door" : "unlock door");
}

static int pcd_tray_move(struct cdrom_device_info *cdi, int position)
{
	char ej_cmd[12] = { 0x1b, 0, 0, 0, 3 - position, 0, 0, 0, 0, 0, 0, 0 };

	return pcd_atapi(cdi->handle, ej_cmd, 0, pcd_scratch,
			 position ? "eject" : "close tray");
}

static void pcd_sleep(int cs)
{
	schedule_timeout_interruptible(cs);
}

static int pcd_reset(struct pcd_unit *cd)
{
	int i, k, flg;
	int expect[5] = { 1, 1, 1, 0x14, 0xeb };

	pi_connect(cd->pi);
	write_reg(cd, 6, 0xa0 + 0x10 * cd->drive);
	write_reg(cd, 7, 8);

	pcd_sleep(20 * HZ / 1000);	/* delay a bit */

	k = 0;
	while ((k++ < PCD_RESET_TMO) && (status_reg(cd) & IDE_BUSY))
		pcd_sleep(HZ / 10);

	flg = 1;
	for (i = 0; i < 5; i++)
		flg &= (read_reg(cd, i + 1) == expect[i]);

	if (verbose) {
		printk("%s: Reset (%d) signature = ", cd->name, k);
		for (i = 0; i < 5; i++)
			printk("%3x", read_reg(cd, i + 1));
		if (!flg)
			printk(" (incorrect)");
		printk("\n");
	}

	pi_disconnect(cd->pi);
	return flg - 1;
}

static int pcd_drive_reset(struct cdrom_device_info *cdi)
{
	return pcd_reset(cdi->handle);
}

static int pcd_ready_wait(struct pcd_unit *cd, int tmo)
{
	char tr_cmd[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int k, p;

	k = 0;
	while (k < tmo) {
		cd->last_sense = 0;
		pcd_atapi(cd, tr_cmd, 0, NULL, DBMSG("test unit ready"));
		p = cd->last_sense;
		if (!p)
			return 0;
		if (!(((p & 0xffff) == 0x0402) || ((p & 0xff) == 6)))
			return p;
		k++;
		pcd_sleep(HZ);
	}
	return 0x000020;	/* timeout */
}

static int pcd_drive_status(struct cdrom_device_info *cdi, int slot_nr)
{
	char rc_cmd[12] = { 0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct pcd_unit *cd = cdi->handle;

	if (pcd_ready_wait(cd, PCD_READY_TMO))
		return CDS_DRIVE_NOT_READY;
	if (pcd_atapi(cd, rc_cmd, 8, pcd_scratch, DBMSG("check media")))
		return CDS_NO_DISC;
	return CDS_DISC_OK;
}

static int pcd_identify(struct pcd_unit *cd, char *id)
{
	int k, s;
	char id_cmd[12] = { 0x12, 0, 0, 0, 36, 0, 0, 0, 0, 0, 0, 0 };

	pcd_bufblk = -1;

	s = pcd_atapi(cd, id_cmd, 36, pcd_buffer, "identify");

	if (s)
		return -1;
	if ((pcd_buffer[0] & 0x1f) != 5) {
		if (verbose)
			printk("%s: %s is not a CD-ROM\n",
			       cd->name, cd->drive ? "Slave" : "Master");
		return -1;
	}
	memcpy(id, pcd_buffer + 16, 16);
	id[16] = 0;
	k = 16;
	while ((k >= 0) && (id[k] <= 0x20)) {
		id[k] = 0;
		k--;
	}

	printk("%s: %s: %s\n", cd->name, cd->drive ? "Slave" : "Master", id);

	return 0;
}

/*
 * returns  0, with id set if drive is detected
 *	    -1, if drive detection failed
 */
static int pcd_probe(struct pcd_unit *cd, int ms, char *id)
{
	if (ms == -1) {
		for (cd->drive = 0; cd->drive <= 1; cd->drive++)
			if (!pcd_reset(cd) && !pcd_identify(cd, id))
				return 0;
	} else {
		cd->drive = ms;
		if (!pcd_reset(cd) && !pcd_identify(cd, id))
			return 0;
	}
	return -1;
}

static void pcd_probe_capabilities(void)
{
	int unit, r;
	char buffer[32];
	char cmd[12] = { 0x5a, 1 << 3, 0x2a, 0, 0, 0, 0, 18, 0, 0, 0, 0 };
	struct pcd_unit *cd;

	for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++) {
		if (!cd->present)
			continue;
		r = pcd_atapi(cd, cmd, 18, buffer, "mode sense capabilities");
		if (r)
			continue;
		/* we should now have the cap page */
		if ((buffer[11] & 1) == 0)
			cd->info.mask |= CDC_CD_R;
		if ((buffer[11] & 2) == 0)
			cd->info.mask |= CDC_CD_RW;
		if ((buffer[12] & 1) == 0)
			cd->info.mask |= CDC_PLAY_AUDIO;
		if ((buffer[14] & 1) == 0)
			cd->info.mask |= CDC_LOCK;
		if ((buffer[14] & 8) == 0)
			cd->info.mask |= CDC_OPEN_TRAY;
		if ((buffer[14] >> 6) == 0)
			cd->info.mask |= CDC_CLOSE_TRAY;
	}
}

static int pcd_detect(void)
{
	char id[18];
	int k, unit;
	struct pcd_unit *cd;

	printk("%s: %s version %s, major %d, nice %d\n",
	       name, name, PCD_VERSION, major, nice);

	par_drv = pi_register_driver(name);
	if (!par_drv) {
		pr_err("failed to register %s driver\n", name);
		return -1;
	}

	k = 0;
	if (pcd_drive_count == 0) { /* nothing spec'd - so autoprobe for 1 */
		cd = pcd;
		if (pi_init(cd->pi, 1, -1, -1, -1, -1, -1, pcd_buffer,
			    PI_PCD, verbose, cd->name)) {
			if (!pcd_probe(cd, -1, id) && cd->disk) {
				cd->present = 1;
				k++;
			} else
				pi_release(cd->pi);
		}
	} else {
		for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++) {
			int *conf = *drives[unit];
			if (!conf[D_PRT])
				continue;
			if (!pi_init(cd->pi, 0, conf[D_PRT], conf[D_MOD],
				     conf[D_UNI], conf[D_PRO], conf[D_DLY],
				     pcd_buffer, PI_PCD, verbose, cd->name)) 
				continue;
			if (!pcd_probe(cd, conf[D_SLV], id) && cd->disk) {
				cd->present = 1;
				k++;
			} else
				pi_release(cd->pi);
		}
	}
	if (k)
		return 0;

	printk("%s: No CD-ROM drive found\n", name);
	for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++)
		put_disk(cd->disk);
	pi_unregister_driver(par_drv);
	return -1;
}

/* I/O request processing */
static struct request_queue *pcd_queue;

static void do_pcd_request(struct request_queue * q)
{
	if (pcd_busy)
		return;
	while (1) {
		if (!pcd_req) {
			pcd_req = blk_fetch_request(q);
			if (!pcd_req)
				return;
		}

		if (rq_data_dir(pcd_req) == READ) {
			struct pcd_unit *cd = pcd_req->rq_disk->private_data;
			if (cd != pcd_current)
				pcd_bufblk = -1;
			pcd_current = cd;
			pcd_sector = blk_rq_pos(pcd_req);
			pcd_count = blk_rq_cur_sectors(pcd_req);
			pcd_buf = bio_data(pcd_req->bio);
			pcd_busy = 1;
			ps_set_intr(do_pcd_read, NULL, 0, nice);
			return;
		} else {
			__blk_end_request_all(pcd_req, -EIO);
			pcd_req = NULL;
		}
	}
}

static inline void next_request(int err)
{
	unsigned long saved_flags;

	spin_lock_irqsave(&pcd_lock, saved_flags);
	if (!__blk_end_request_cur(pcd_req, err))
		pcd_req = NULL;
	pcd_busy = 0;
	do_pcd_request(pcd_queue);
	spin_unlock_irqrestore(&pcd_lock, saved_flags);
}

static int pcd_ready(void)
{
	return (((status_reg(pcd_current) & (IDE_BUSY | IDE_DRQ)) == IDE_DRQ));
}

static void pcd_transfer(void)
{

	while (pcd_count && (pcd_sector / 4 == pcd_bufblk)) {
		int o = (pcd_sector % 4) * 512;
		memcpy(pcd_buf, pcd_buffer + o, 512);
		pcd_count--;
		pcd_buf += 512;
		pcd_sector++;
	}
}

static void pcd_start(void)
{
	int b, i;
	char rd_cmd[12] = { 0xa8, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 };

	pcd_bufblk = pcd_sector / 4;
	b = pcd_bufblk;
	for (i = 0; i < 4; i++) {
		rd_cmd[5 - i] = b & 0xff;
		b = b >> 8;
	}

	if (pcd_command(pcd_current, rd_cmd, 2048, "read block")) {
		pcd_bufblk = -1;
		next_request(-EIO);
		return;
	}

	mdelay(1);

	ps_set_intr(do_pcd_read_drq, pcd_ready, PCD_TMO, nice);
}

static void do_pcd_read(void)
{
	pcd_busy = 1;
	pcd_retries = 0;
	pcd_transfer();
	if (!pcd_count) {
		next_request(0);
		return;
	}

	pi_do_claimed(pcd_current->pi, pcd_start);
}

static void do_pcd_read_drq(void)
{
	unsigned long saved_flags;

	if (pcd_completion(pcd_current, pcd_buffer, "read block")) {
		if (pcd_retries < PCD_RETRIES) {
			mdelay(1);
			pcd_retries++;
			pi_do_claimed(pcd_current->pi, pcd_start);
			return;
		}
		pcd_bufblk = -1;
		next_request(-EIO);
		return;
	}

	do_pcd_read();
	spin_lock_irqsave(&pcd_lock, saved_flags);
	do_pcd_request(pcd_queue);
	spin_unlock_irqrestore(&pcd_lock, saved_flags);
}

/* the audio_ioctl stuff is adapted from sr_ioctl.c */

static int pcd_audio_ioctl(struct cdrom_device_info *cdi, unsigned int cmd, void *arg)
{
	struct pcd_unit *cd = cdi->handle;

	switch (cmd) {

	case CDROMREADTOCHDR:

		{
			char cmd[12] =
			    { GPCMD_READ_TOC_PMA_ATIP, 0, 0, 0, 0, 0, 0, 0, 12,
			 0, 0, 0 };
			struct cdrom_tochdr *tochdr =
			    (struct cdrom_tochdr *) arg;
			char buffer[32];
			int r;

			r = pcd_atapi(cd, cmd, 12, buffer, "read toc header");

			tochdr->cdth_trk0 = buffer[2];
			tochdr->cdth_trk1 = buffer[3];

			return r ? -EIO : 0;
		}

	case CDROMREADTOCENTRY:

		{
			char cmd[12] =
			    { GPCMD_READ_TOC_PMA_ATIP, 0, 0, 0, 0, 0, 0, 0, 12,
			 0, 0, 0 };

			struct cdrom_tocentry *tocentry =
			    (struct cdrom_tocentry *) arg;
			unsigned char buffer[32];
			int r;

			cmd[1] =
			    (tocentry->cdte_format == CDROM_MSF ? 0x02 : 0);
			cmd[6] = tocentry->cdte_track;

			r = pcd_atapi(cd, cmd, 12, buffer, "read toc entry");

			tocentry->cdte_ctrl = buffer[5] & 0xf;
			tocentry->cdte_adr = buffer[5] >> 4;
			tocentry->cdte_datamode =
			    (tocentry->cdte_ctrl & 0x04) ? 1 : 0;
			if (tocentry->cdte_format == CDROM_MSF) {
				tocentry->cdte_addr.msf.minute = buffer[9];
				tocentry->cdte_addr.msf.second = buffer[10];
				tocentry->cdte_addr.msf.frame = buffer[11];
			} else
				tocentry->cdte_addr.lba =
				    (((((buffer[8] << 8) + buffer[9]) << 8)
				      + buffer[10]) << 8) + buffer[11];

			return r ? -EIO : 0;
		}

	default:

		return -ENOSYS;
	}
}

static int pcd_get_mcn(struct cdrom_device_info *cdi, struct cdrom_mcn *mcn)
{
	char cmd[12] =
	    { GPCMD_READ_SUBCHANNEL, 0, 0x40, 2, 0, 0, 0, 0, 24, 0, 0, 0 };
	char buffer[32];

	if (pcd_atapi(cdi->handle, cmd, 24, buffer, "get mcn"))
		return -EIO;

	memcpy(mcn->medium_catalog_number, buffer + 9, 13);
	mcn->medium_catalog_number[13] = 0;

	return 0;
}

static int __init pcd_init(void)
{
	struct pcd_unit *cd;
	int unit;

	if (disable)
		return -EINVAL;

	pcd_init_units();

	if (pcd_detect())
		return -ENODEV;

	/* get the atapi capabilities page */
	pcd_probe_capabilities();

	if (register_blkdev(major, name)) {
		for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++)
			put_disk(cd->disk);
		return -EBUSY;
	}

	pcd_queue = blk_init_queue(do_pcd_request, &pcd_lock);
	if (!pcd_queue) {
		unregister_blkdev(major, name);
		for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++)
			put_disk(cd->disk);
		return -ENOMEM;
	}

	for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++) {
		if (cd->present) {
			register_cdrom(&cd->info);
			cd->disk->private_data = cd;
			cd->disk->queue = pcd_queue;
			add_disk(cd->disk);
		}
	}

	return 0;
}

static void __exit pcd_exit(void)
{
	struct pcd_unit *cd;
	int unit;

	for (unit = 0, cd = pcd; unit < PCD_UNITS; unit++, cd++) {
		if (cd->present) {
			del_gendisk(cd->disk);
			pi_release(cd->pi);
			unregister_cdrom(&cd->info);
		}
		put_disk(cd->disk);
	}
	blk_cleanup_queue(pcd_queue);
	unregister_blkdev(major, name);
	pi_unregister_driver(par_drv);
}

MODULE_LICENSE("GPL");
module_init(pcd_init)
module_exit(pcd_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /* 
        pd.c    (c) 1997-8  Grant R. Guenther <grant@torque.net>
                            Under the terms of the GNU General Public License.

        This is the high-level driver for parallel port IDE hard
        drives based on chips supported by the paride module.

	By default, the driver will autoprobe for a single parallel
	port IDE drive, but if their individual parameters are
        specified, the driver can handle up to 4 drives.

        The behaviour of the pd driver can be altered by setting
        some parameters from the insmod command line.  The following
        parameters are adjustable:
 
	    drive0  	These four arguments can be arrays of	    
	    drive1	1-8 integers as follows:
	    drive2
	    drive3	<prt>,<pro>,<uni>,<mod>,<geo>,<sby>,<dly>,<slv>

			Where,

		<prt>	is the base of the parallel port address for
			the corresponding drive.  (required)

		<pro>   is the protocol number for the adapter that
			supports this drive.  These numbers are
                        logged by 'paride' when the protocol modules
			are initialised.  (0 if not given)

		<uni>   for those adapters that support chained
			devices, this is the unit selector for the
		        chain of devices on the given port.  It should
			be zero for devices that don't support chaining.
			(0 if not given)

		<mod>   this can be -1 to choose the best mode, or one
		        of the mode numbers supported by the adapter.
			(-1 if not given)

		<geo>   this defaults to 0 to indicate that the driver
			should use the CHS geometry provided by the drive
			itself.  If set to 1, the driver will provide
			a logical geometry with 64 heads and 32 sectors
			per track, to be consistent with most SCSI
		        drivers.  (0 if not given)

		<sby>   set this to zero to disable the power saving
			standby mode, if needed.  (1 if not given)

		<dly>   some parallel ports require the driver to 
			go more slowly.  -1 sets a default value that
			should work with the chosen protocol.  Otherwise,
			set this to a small integer, the larger it is
			the slower the port i/o.  In some cases, setting
			this to zero will speed up the device. (default -1)

		<slv>   IDE disks can be jumpered to master or slave.
                        Set this to 0 to choose the master drive, 1 to
                        choose the slave, -1 (the default) to choose the
                        first drive found.
			

            major       You may use this parameter to overide the
                        default major number (45) that this driver
                        will use.  Be sure to change the device
                        name as well.

            name        This parameter is a character string that
                        contains the name the kernel will use for this
                        device (in /proc output, for instance).
			(default "pd")

	    cluster	The driver will attempt to aggregate requests
			for adjacent blocks into larger multi-block
			clusters.  The maximum cluster size (in 512
			byte sectors) is set with this parameter.
			(default 64)

	    verbose	This parameter controls the amount of logging
			that the driver will do.  Set it to 0 for 
			normal operation, 1 to see autoprobe progress
			messages, or 2 to see additional debugging
			output.  (default 0)

            nice        This parameter controls the driver's use of
                        idle CPU time, at the expense of some speed.

        If this driver is built into the kernel, you can use kernel
        the following command line parameters, with the same values
        as the corresponding module parameters listed above:

            pd.drive0
            pd.drive1
            pd.drive2
            pd.drive3
            pd.cluster
            pd.nice

        In addition, you can use the parameter pd.disable to disable
        the driver entirely.
 
*/

/* Changes:

	1.01	GRG 1997.01.24	Restored pd_reset()
				Added eject ioctl
	1.02    GRG 1998.05.06  SMP spinlock changes, 
				Added slave support
	1.03    GRG 1998.06.16  Eliminate an Ugh.
	1.04	GRG 1998.08.15  Extra debugging, use HZ in loop timing
	1.05    GRG 1998.09.24  Added jumbo support

*/

#define PD_VERSION      "1.05"
#define PD_MAJOR	45
#define PD_NAME		"pd"
#define PD_UNITS	4

/* Here are things one can override from the insmod command.
   Most are autoprobed by paride unless set here.  Verbose is off
   by default.

*/
#include <linux/types.h>

static int verbose = 0;
static int major = PD_MAJOR;
static char *name = PD_NAME;
static int cluster = 64;
static int nice = 0;
static int disable = 0;

static int drive0[8] = { 0, 0, 0, -1, 0, 1, -1, -1 };
static int drive1[8] = { 0, 0, 0, -1, 0, 1, -1, -1 };
static int drive2[8] = { 0, 0, 0, -1, 0, 1, -1, -1 };
static int drive3[8] = { 0, 0, 0, -1, 0, 1, -1, -1 };

static int (*drives[4])[8] = {&drive0, &drive1, &drive2, &drive3};

enum {D_PRT, D_PRO, D_UNI, D_MOD, D_GEO, D_SBY, D_DLY, D_SLV};

/* end of parameters */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/gfp.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/hdreg.h>
#include <linux/cdrom.h>	/* for the eject ioctl */
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>

static DEFINE_MUTEX(pd_mutex);
static DEFINE_SPINLOCK(pd_lock);

module_param(verbose, int, 0);
module_param(major, int, 0);
module_param(name, charp, 0);
module_param(cluster, int, 0);
module_param(nice, int, 0);
module_param_array(drive0, int, NULL, 0);
module_param_array(drive1, int, NULL, 0);
module_param_array(drive2, int, NULL, 0);
module_param_array(drive3, int, NULL, 0);

#include "paride.h"

#define PD_BITS    4

/* numbers for "SCSI" geometry */

#define PD_LOG_HEADS    64
#define PD_LOG_SECTS    32

#define PD_ID_OFF       54
#define PD_ID_LEN       14

#define PD_MAX_RETRIES  5
#define PD_TMO          800	/* interrupt timeout in jiffies */
#define PD_SPIN_DEL     50	/* spin delay in micro-seconds  */

#define PD_SPIN         (1000000*PD_TMO)/(HZ*PD_SPIN_DEL)

#define STAT_ERR        0x00001
#define STAT_INDEX      0x00002
#define STAT_ECC        0x00004
#define STAT_DRQ        0x00008
#define STAT_SEEK       0x00010
#define STAT_WRERR      0x00020
#define STAT_READY      0x00040
#define STAT_BUSY       0x00080

#define ERR_AMNF        0x00100
#define ERR_TK0NF       0x00200
#define ERR_ABRT        0x00400
#define ERR_MCR         0x00800
#define ERR_IDNF        0x01000
#define ERR_MC          0x02000
#define ERR_UNC         0x04000
#define ERR_TMO         0x10000

#define IDE_READ        	0x20
#define IDE_WRITE       	0x30
#define IDE_READ_VRFY		0x40
#define IDE_INIT_DEV_PARMS	0x91
#define IDE_STANDBY     	0x96
#define IDE_ACKCHANGE   	0xdb
#define IDE_DOORLOCK    	0xde
#define IDE_DOORUNLOCK  	0xdf
#define IDE_IDENTIFY    	0xec
#define IDE_EJECT		0xed

#define PD_NAMELEN	8

struct pd_unit {
	struct pi_adapter pia;	/* interface to paride layer */
	struct pi_adapter *pi;
	int access;		/* count of active opens ... */
	int capacity;		/* Size of this volume in sectors */
	int heads;		/* physical geometry */
	int sectors;
	int cylinders;
	int can_lba;
	int drive;		/* master=0 slave=1 */
	int changed;		/* Have we seen a disk change ? */
	int removable;		/* removable media device  ?  */
	int standby;
	int alt_geom;
	char name[PD_NAMELEN];	/* pda, pdb, etc ... */
	struct gendisk *gd;
};

static struct pd_unit pd[PD_UNITS];

static char pd_scratch[512];	/* scratch block buffer */

static char *pd_errs[17] = { "ERR", "INDEX", "ECC", "DRQ", "SEEK", "WRERR",
	"READY", "BUSY", "AMNF", "TK0NF", "ABRT", "MCR",
	"IDNF", "MC", "UNC", "???", "TMO"
};

static void *par_drv;		/* reference of parport driver */

static inline int status_reg(struct pd_unit *disk)
{
	return pi_read_regr(disk->pi, 1, 6);
}

static inline int read_reg(struct pd_unit *disk, int reg)
{
	return pi_read_regr(disk->pi, 0, reg);
}

static inline void write_status(struct pd_unit *disk, int val)
{
	pi_write_regr(disk->pi, 1, 6, val);
}

static inline void write_reg(struct pd_unit *disk, int reg, int val)
{
	pi_write_regr(disk->pi, 0, reg, val);
}

static inline u8 DRIVE(struct pd_unit *disk)
{
	return 0xa0+0x10*disk->drive;
}

/*  ide command interface */

static void pd_print_error(struct pd_unit *disk, char *msg, int status)
{
	int i;

	printk("%s: %s: status = 0x%x =", disk->name, msg, status);
	for (i = 0; i < ARRAY_SIZE(pd_errs); i++)
		if (status & (1 << i))
			printk(" %s", pd_errs[i]);
	printk("\n");
}

static void pd_reset(struct pd_unit *disk)
{				/* called only for MASTER drive */
	write_status(disk, 4);
	udelay(50);
	write_status(disk, 0);
	udelay(250);
}

#define DBMSG(msg)	((verbose>1)?(msg):NULL)

static int pd_wait_for(struct pd_unit *disk, int w, char *msg)
{				/* polled wait */
	int k, r, e;

	k = 0;
	while (k < PD_SPIN) {
		r = status_reg(disk);
		k++;
		if (((r & w) == w) && !(r & STAT_BUSY))
			break;
		udelay(PD_SPIN_DEL);
	}
	e = (read_reg(disk, 1) << 8) + read_reg(disk, 7);
	if (k >= PD_SPIN)
		e |= ERR_TMO;
	if ((e & (STAT_ERR | ERR_TMO)) && (msg != NULL))
		pd_print_error(disk, msg, e);
	return e;
}

static void pd_send_command(struct pd_unit *disk, int n, int s, int h, int c0, int c1, int func)
{
	write_reg(disk, 6, DRIVE(disk) + h);
	write_reg(disk, 1, 0);		/* the IDE task file */
	write_reg(disk, 2, n);
	write_reg(disk, 3, s);
	write_reg(disk, 4, c0);
	write_reg(disk, 5, c1);
	write_reg(disk, 7, func);

	udelay(1);
}

static void pd_ide_command(struct pd_unit *disk, int func, int block, int count)
{
	int c1, c0, h, s;

	if (disk->can_lba) {
		s = block & 255;
		c0 = (block >>= 8) & 255;
		c1 = (block >>= 8) & 255;
		h = ((block >>= 8) & 15) + 0x40;
	} else {
		s = (block % disk->sectors) + 1;
		h = (block /= disk->sectors) % disk->heads;
		c0 = (block /= disk->heads) % 256;
		c1 = (block >>= 8);
	}
	pd_send_command(disk, count, s, h, c0, c1, func);
}

/* The i/o request engine */

enum action {Fail = 0, Ok = 1, Hold, Wait};

static struct request *pd_req;	/* current request */
static enum action (*phase)(void);

static void run_fsm(void);

static void ps_tq_int(struct work_struct *work);

static DECLARE_DELAYED_WORK(fsm_tq, ps_tq_int);

static void schedule_fsm(void)
{
	if (!nice)
		schedule_delayed_work(&fsm_tq, 0);
	else
		schedule_delayed_work(&fsm_tq, nice-1);
}

static void ps_tq_int(struct work_struct *work)
{
	run_fsm();
}

static enum action do_pd_io_start(void);
static enum action pd_special(void);
static enum action do_pd_read_start(void);
static enum action do_pd_write_start(void);
static enum action do_pd_read_drq(void);
static enum action do_pd_write_done(void);

static struct request_queue *pd_queue;
static int pd_claimed;

static struct pd_unit *pd_current; /* current request's drive */
static PIA *pi_current; /* current request's PIA */

static void run_fsm(void)
{
	while (1) {
		enum action res;
		unsigned long saved_flags;
		int stop = 0;

		if (!phase) {
			pd_current = pd_req->rq_disk->private_data;
			pi_current = pd_current->pi;
			phase = do_pd_io_start;
		}

		switch (pd_claimed) {
			case 0:
				pd_claimed = 1;
				if (!pi_schedule_claimed(pi_current, run_fsm))
					return;
			case 1:
				pd_claimed = 2;
				pi_current->proto->connect(pi_current);
		}

		switch(res = phase()) {
			case Ok: case Fail:
				pi_disconnect(pi_current);
				pd_claimed = 0;
				phase = NULL;
				spin_lock_irqsave(&pd_lock, saved_flags);
				if (!__blk_end_request_cur(pd_req,
						res == Ok ? 0 : -EIO)) {
					pd_req = blk_fetch_request(pd_queue);
					if (!pd_req)
						stop = 1;
				}
				spin_unlock_irqrestore(&pd_lock, saved_flags);
				if (stop)
					return;
			case Hold:
				schedule_fsm();
				return;
			case Wait:
				pi_disconnect(pi_current);
				pd_claimed = 0;
		}
	}
}

static int pd_retries = 0;	/* i/o error retry count */
static int pd_block;		/* address of next requested block */
static int pd_count;		/* number of blocks still to do */
static int pd_run;		/* sectors in current cluster */
static int pd_cmd;		/* current command READ/WRITE */
static char *pd_buf;		/* buffer for request in progress */

static enum action do_pd_io_start(void)
{
	if (pd_req->cmd_type == REQ_TYPE_DRV_PRIV) {
		phase = pd_special;
		return pd_special();
	}

	pd_cmd = rq_data_dir(pd_req);
	if (pd_cmd == READ || pd_cmd == WRITE) {
		pd_block = blk_rq_pos(pd_req);
		pd_count = blk_rq_cur_sectors(pd_req);
		if (pd_block + pd_count > get_capacity(pd_req->rq_disk))
			return Fail;
		pd_run = blk_rq_sectors(pd_req);
		pd_buf = bio_data(pd_req->bio);
		pd_retries = 0;
		if (pd_cmd == READ)
			return do_pd_read_start();
		else
			return do_pd_write_start();
	}
	return Fail;
}

static enum action pd_special(void)
{
	enum action (*func)(struct pd_unit *) = pd_req->special;
	return func(pd_current);
}

static int pd_next_buf(void)
{
	unsigned long saved_flags;

	pd_count--;
	pd_run--;
	pd_buf += 512;
	pd_block++;
	if (!pd_run)
		return 1;
	if (pd_count)
		return 0;
	spin_lock_irqsave(&pd_lock, saved_flags);
	__blk_end_request_cur(pd_req, 0);
	pd_count = blk_rq_cur_sectors(pd_req);
	pd_buf = bio_data(pd_req->bio);
	spin_unlock_irqrestore(&pd_lock, saved_flags);
	return 0;
}

static unsigned long pd_timeout;

static enum action do_pd_read_start(void)
{
	if (pd_wait_for(pd_current, STAT_READY, "do_pd_read") & STAT_ERR) {
		if (pd_retries < PD_MAX_RETRIES) {
			pd_retries++;
			return Wait;
		}
		return Fail;
	}
	pd_ide_command(pd_current, IDE_READ, pd_block, pd_run);
	phase = do_pd_read_drq;
	pd_timeout = jiffies + PD_TMO;
	return Hold;
}

static enum action do_pd_write_start(void)
{
	if (pd_wait_for(pd_current, STAT_READY, "do_pd_write") & STAT_ERR) {
		if (pd_retries < PD_MAX_RETRIES) {
			pd_retries++;
			return Wait;
		}
		return Fail;
	}
	pd_ide_command(pd_current, IDE_WRITE, pd_block, pd_run);
	while (1) {
		if (pd_wait_for(pd_current, STAT_DRQ, "do_pd_write_drq") & STAT_ERR) {
			if (pd_retries < PD_MAX_RETRIES) {
				pd_retries++;
				return Wait;
			}
			return Fail;
		}
		pi_write_block(pd_current->pi, pd_buf, 512);
		if (pd_next_buf())
			break;
	}
	phase = do_pd_write_done;
	pd_timeout = jiffies + PD_TMO;
	return Hold;
}

static inline int pd_ready(void)
{
	return !(status_reg(pd_current) & STAT_BUSY);
}

static enum action do_pd_read_drq(void)
{
	if (!pd_ready() && !time_after_eq(jiffies, pd_timeout))
		return Hold;

	while (1) {
		if (pd_wait_for(pd_current, STAT_DRQ, "do_pd_read_drq") & STAT_ERR) {
			if (pd_retries < PD_MAX_RETRIES) {
				pd_retries++;
				phase = do_pd_read_start;
				return Wait;
			}
			return Fail;
		}
		pi_read_block(pd_current->pi, pd_buf, 512);
		if (pd_next_buf())
			break;
	}
	return Ok;
}

static enum action do_pd_write_done(void)
{
	if (!pd_ready() && !time_after_eq(jiffies, pd_timeout))
		return Hold;

	if (pd_wait_for(pd_current, STAT_READY, "do_pd_write_done") & STAT_ERR) {
		if (pd_retries < PD_MAX_RETRIES) {
			pd_retries++;
			phase = do_pd_write_start;
			return Wait;
		}
		return Fail;
	}
	return Ok;
}

/* special io requests */

/* According to the ATA standard, the default CHS geometry should be
   available following a reset.  Some Western Digital drives come up
   in a mode where only LBA addresses are accepted until the device
   parameters are initialised.
*/

static void pd_init_dev_parms(struct pd_unit *disk)
{
	pd_wait_for(disk, 0, DBMSG("before init_dev_parms"));
	pd_send_command(disk, disk->sectors, 0, disk->heads - 1, 0, 0,
			IDE_INIT_DEV_PARMS);
	udelay(300);
	pd_wait_for(disk, 0, "Initialise device parameters");
}

static enum action pd_door_lock(struct pd_unit *disk)
{
	if (!(pd_wait_for(disk, STAT_READY, "Lock") & STAT_ERR)) {
		pd_send_command(disk, 1, 0, 0, 0, 0, IDE_DOORLOCK);
		pd_wait_for(disk, STAT_READY, "Lock done");
	}
	return Ok;
}

static enum action pd_door_unlock(struct pd_unit *disk)
{
	if (!(pd_wait_for(disk, STAT_READY, "Lock") & STAT_ERR)) {
		pd_send_command(disk, 1, 0, 0, 0, 0, IDE_DOORUNLOCK);
		pd_wait_for(disk, STAT_READY, "Lock done");
	}
	return Ok;
}

static enum action pd_eject(struct pd_unit *disk)
{
	pd_wait_for(disk, 0, DBMSG("before unlock on eject"));
	pd_send_command(disk, 1, 0, 0, 0, 0, IDE_DOORUNLOCK);
	pd_wait_for(disk, 0, DBMSG("after unlock on eject"));
	pd_wait_for(disk, 0, DBMSG("before eject"));
	pd_send_command(disk, 0, 0, 0, 0, 0, IDE_EJECT);
	pd_wait_for(disk, 0, DBMSG("after eject"));
	return Ok;
}

static enum action pd_media_check(struct pd_unit *disk)
{
	int r = pd_wait_for(disk, STAT_READY, DBMSG("before media_check"));
	if (!(r & STAT_ERR)) {
		pd_send_command(disk, 1, 1, 0, 0, 0, IDE_READ_VRFY);
		r = pd_wait_for(disk, STAT_READY, DBMSG("RDY after READ_VRFY"));
	} else
		disk->changed = 1;	/* say changed if other error */
	if (r & ERR_MC) {
		disk->changed = 1;
		pd_send_command(disk, 1, 0, 0, 0, 0, IDE_ACKCHANGE);
		pd_wait_for(disk, STAT_READY, DBMSG("RDY after ACKCHANGE"));
		pd_send_command(disk, 1, 1, 0, 0, 0, IDE_READ_VRFY);
		r = pd_wait_for(disk, STAT_READY, DBMSG("RDY after VRFY"));
	}
	return Ok;
}

static void pd_standby_off(struct pd_unit *disk)
{
	pd_wait_for(disk, 0, DBMSG("before STANDBY"));
	pd_send_command(disk, 0, 0, 0, 0, 0, IDE_STANDBY);
	pd_wait_for(disk, 0, DBMSG("after STANDBY"));
}

static enum action pd_identify(struct pd_unit *disk)
{
	int j;
	char id[PD_ID_LEN + 1];

/* WARNING:  here there may be dragons.  reset() applies to both drives,
   but we call it only on probing the MASTER. This should allow most
   common configurations to work, but be warned that a reset can clear
   settings on the SLAVE drive.
*/

	if (disk->drive == 0)
		pd_reset(disk);

	write_reg(disk, 6, DRIVE(disk));
	pd_wait_for(disk, 0, DBMSG("before IDENT"));
	pd_send_command(disk, 1, 0, 0, 0, 0, IDE_IDENTIFY);

	if (pd_wait_for(disk, STAT_DRQ, DBMSG("IDENT DRQ")) & STAT_ERR)
		return Fail;
	pi_read_block(disk->pi, pd_scratch, 512);
	disk->can_lba = pd_scratch[99] & 2;
	disk->sectors = le16_to_cpu(*(__le16 *) (pd_scratch + 12));
	disk->heads = le16_to_cpu(*(__le16 *) (pd_scratch + 6));
	disk->cylinders = le16_to_cpu(*(__le16 *) (pd_scratch + 2));
	if (disk->can_lba)
		disk->capacity = le32_to_cpu(*(__le32 *) (pd_scratch + 120));
	else
		disk->capacity = disk->sectors * disk->heads * disk->cylinders;

	for (j = 0; j < PD_ID_LEN; j++)
		id[j ^ 1] = pd_scratch[j + PD_ID_OFF];
	j = PD_ID_LEN - 1;
	while ((j >= 0) && (id[j] <= 0x20))
		j--;
	j++;
	id[j] = 0;

	disk->removable = pd_scratch[0] & 0x80;

	printk("%s: %s, %s, %d blocks [%dM], (%d/%d/%d), %s media\n",
	       disk->name, id,
	       disk->drive ? "slave" : "master",
	       disk->capacity, disk->capacity / 2048,
	       disk->cylinders, disk->heads, disk->sectors,
	       disk->removable ? "removable" : "fixed");

	if (disk->capacity)
		pd_init_dev_parms(disk);
	if (!disk->standby)
		pd_standby_off(disk);

	return Ok;
}

/* end of io request engine */

static void do_pd_request(struct request_queue * q)
{
	if (pd_req)
		return;
	pd_req = blk_fetch_request(q);
	if (!pd_req)
		return;

	schedule_fsm();
}

static int pd_special_command(struct pd_unit *disk,
		      enum action (*func)(struct pd_unit *disk))
{
	struct request *rq;
	int err = 0;

	rq = blk_get_request(disk->gd->queue, READ, __GFP_RECLAIM);
	if (IS_ERR(rq))
		return PTR_ERR(rq);

	rq->cmd_type = REQ_TYPE_DRV_PRIV;
	rq->special = func;

	err = blk_execute_rq(disk->gd->queue, disk->gd, rq, 0);

	blk_put_request(rq);
	return err;
}

/* kernel glue structures */

static int pd_open(struct block_device *bdev, fmode_t mode)
{
	struct pd_unit *disk = bdev->bd_disk->private_data;

	mutex_lock(&pd_mutex);
	disk->access++;

	if (disk->removable) {
		pd_special_command(disk, pd_media_check);
		pd_special_command(disk, pd_door_lock);
	}
	mutex_unlock(&pd_mutex);
	return 0;
}

static int pd_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct pd_unit *disk = bdev->bd_disk->private_data;

	if (disk->alt_geom) {
		geo->heads = PD_LOG_HEADS;
		geo->sectors = PD_LOG_SECTS;
		geo->cylinders = disk->capacity / (geo->heads * geo->sectors);
	} else {
		geo->heads = disk->heads;
		geo->sectors = disk->sectors;
		geo->cylinders = disk->cylinders;
	}

	return 0;
}

static int pd_ioctl(struct block_device *bdev, fmode_t mode,
	 unsigned int cmd, unsigned long arg)
{
	struct pd_unit *disk = bdev->bd_disk->private_data;

	switch (cmd) {
	case CDROMEJECT:
		mutex_lock(&pd_mutex);
		if (disk->access == 1)
			pd_special_command(disk, pd_eject);
		mutex_unlock(&pd_mutex);
		return 0;
	default:
		return -EINVAL;
	}
}

static void pd_release(struct gendisk *p, fmode_t mode)
{
	struct pd_unit *disk = p->private_data;

	mutex_lock(&pd_mutex);
	if (!--disk->access && disk->removable)
		pd_special_command(disk, pd_door_unlock);
	mutex_unlock(&pd_mutex);
}

static unsigned int pd_check_events(struct gendisk *p, unsigned int clearing)
{
	struct pd_unit *disk = p->private_data;
	int r;
	if (!disk->removable)
		return 0;
	pd_special_command(disk, pd_media_check);
	r = disk->changed;
	disk->changed = 0;
	return r ? DISK_EVENT_MEDIA_CHANGE : 0;
}

static int pd_revalidate(struct gendisk *p)
{
	struct pd_unit *disk = p->private_data;
	if (pd_special_command(disk, pd_identify) == 0)
		set_capacity(p, disk->capacity);
	else
		set_capacity(p, 0);
	return 0;
}

static const struct block_device_operations pd_fops = {
	.owner		= THIS_MODULE,
	.open		= pd_open,
	.release	= pd_release,
	.ioctl		= pd_ioctl,
	.getgeo		= pd_getgeo,
	.check_events	= pd_check_events,
	.revalidate_disk= pd_revalidate
};

/* probing */

static void pd_probe_drive(struct pd_unit *disk)
{
	struct gendisk *p = alloc_disk(1 << PD_BITS);
	if (!p)
		return;
	strcpy(p->disk_name, disk->name);
	p->fops = &pd_fops;
	p->major = major;
	p->first_minor = (disk - pd) << PD_BITS;
	disk->gd = p;
	p->private_data = disk;
	p->queue = pd_queue;

	if (disk->drive == -1) {
		for (disk->drive = 0; disk->drive <= 1; disk->drive++)
			if (pd_special_command(disk, pd_identify) == 0)
				return;
	} else if (pd_special_command(disk, pd_identify) == 0)
		return;
	disk->gd = NULL;
	put_disk(p);
}

static int pd_detect(void)
{
	int found = 0, unit, pd_drive_count = 0;
	struct pd_unit *disk;

	for (unit = 0; unit < PD_UNITS; unit++) {
		int *parm = *drives[unit];
		struct pd_unit *disk = pd + unit;
		disk->pi = &disk->pia;
		disk->access = 0;
		disk->changed = 1;
		disk->capacity = 0;
		disk->drive = parm[D_SLV];
		snprintf(disk->name, PD_NAMELEN, "%s%c", name, 'a'+unit);
		disk->alt_geom = parm[D_GEO];
		disk->standby = parm[D_SBY];
		if (parm[D_PRT])
			pd_drive_count++;
	}

	par_drv = pi_register_driver(name);
	if (!par_drv) {
		pr_err("failed to register %s driver\n", name);
		return -1;
	}

	if (pd_drive_count == 0) { /* nothing spec'd - so autoprobe for 1 */
		disk = pd;
		if (pi_init(disk->pi, 1, -1, -1, -1, -1, -1, pd_scratch,
			    PI_PD, verbose, disk->name)) {
			pd_probe_drive(disk);
			if (!disk->gd)
				pi_release(disk->pi);
		}

	} else {
		for (unit = 0, disk = pd; unit < PD_UNITS; unit++, disk++) {
			int *parm = *drives[unit];
			if (!parm[D_PRT])
				continue;
			if (pi_init(disk->pi, 0, parm[D_PRT], parm[D_MOD],
				     parm[D_UNI], parm[D_PRO], parm[D_DLY],
				     pd_scratch, PI_PD, verbose, disk->name)) {
				pd_probe_drive(disk);
				if (!disk->gd)
					pi_release(disk->pi);
			}
		}
	}
	for (unit = 0, disk = pd; unit < PD_UNITS; unit++, disk++) {
		if (disk->gd) {
			set_capacity(disk->gd, disk->capacity);
			add_disk(disk->gd);
			found = 1;
		}
	}
	if (!found) {
		printk("%s: no valid drive found\n", name);
		pi_unregister_driver(par_drv);
	}
	return found;
}

static int __init pd_init(void)
{
	if (disable)
		goto out1;

	pd_queue = blk_init_queue(do_pd_request, &pd_lock);
	if (!pd_queue)
		goto out1;

	blk_queue_max_hw_sectors(pd_queue, cluster);

	if (register_blkdev(major, name))
		goto out2;

	printk("%s: %s version %s, major %d, cluster %d, nice %d\n",
	       name, name, PD_VERSION, major, cluster, nice);
	if (!pd_detect())
		goto out3;

	return 0;

out3:
	unregister_blkdev(major, name);
out2:
	blk_cleanup_queue(pd_queue);
out1:
	return -ENODEV;
}

static void __exit pd_exit(void)
{
	struct pd_unit *disk;
	int unit;
	unregister_blkdev(major, name);
	for (unit = 0, disk = pd; unit < PD_UNITS; unit++, disk++) {
		struct gendisk *p = disk->gd;
		if (p) {
			disk->gd = NULL;
			del_gendisk(p);
			put_disk(p);
			pi_release(disk->pi);
		}
	}
	blk_cleanup_queue(pd_queue);
}

MODULE_LICENSE("GPL");
module_init(pd_init)
module_exit(pd_exit)
                                                                                              /* 
        pf.c    (c) 1997-8  Grant R. Guenther <grant@torque.net>
                            Under the terms of the GNU General Public License.

        This is the high-level driver for parallel port ATAPI disk
        drives based on chips supported by the paride module.

        By default, the driver will autoprobe for a single parallel
        port ATAPI disk drive, but if their individual parameters are
        specified, the driver can handle up to 4 drives.

        The behaviour of the pf driver can be altered by setting
        some parameters from the insmod command line.  The following
        parameters are adjustable:

            drive0      These four arguments can be arrays of       
            drive1      1-7 integers as follows:
            drive2
            drive3      <prt>,<pro>,<uni>,<mod>,<slv>,<lun>,<dly>

                        Where,

                <prt>   is the base of the parallel port address for
                        the corresponding drive.  (required)

                <pro>   is the protocol number for the adapter that
                        supports this drive.  These numbers are
                        logged by 'paride' when the protocol modules
                        are initialised.  (0 if not given)

                <uni>   for those adapters that support chained
                        devices, this is the unit selector for the
                        chain of devices on the given port.  It should
                        be zero for devices that don't support chaining.
                        (0 if not given)

                <mod>   this can be -1 to choose the best mode, or one
                        of the mode numbers supported by the adapter.
                        (-1 if not given)

                <slv>   ATAPI CDroms can be jumpered to master or slave.
                        Set this to 0 to choose the master drive, 1 to
                        choose the slave, -1 (the default) to choose the
                        first drive found.

		<lun>   Some ATAPI devices support multiple LUNs.
                        One example is the ATAPI PD/CD drive from
                        Matshita/Panasonic.  This device has a 
                        CD drive on LUN 0 and a PD drive on LUN 1.
                        By default, the driver will search for the
                        first LUN with a supported device.  Set 
                        this parameter to force it to use a specific
                        LUN.  (default -1)

                <dly>   some parallel ports require the driver to 
                        go more slowly.  -1 sets a default value that
                        should work with the chosen protocol.  Otherwise,
                        set this to a small integer, the larger it is
                        the slower the port i/o.  In some cases, setting
                        this to zero will speed up the device. (default -1)

	    major	You may use this parameter to overide the
			default major number (47) that this driver
			will use.  Be sure to change the device
			name as well.

	    name	This parameter is a character string that
			contains the name the kernel will use for this
			device (in /proc output, for instance).
			(default "pf").

            cluster     The driver will attempt to aggregate requests
                        for adjacent blocks into larger multi-block
                        clusters.  The maximum cluster size (in 512
                        byte sectors) is set with this parameter.
                        (default 64)

            verbose     This parameter controls the amount of logging
                        that the driver will do.  Set it to 0 for
                        normal operation, 1 to see autoprobe progress
                        messages, or 2 to see additional debugging
                        output.  (default 0)
 
	    nice        This parameter controls the driver's use of
			idle CPU time, at the expense of some speed.

        If this driver is built into the kernel, you can use the
        following command line parameters, with the same values
        as the corresponding module parameters listed above:

            pf.drive0
            pf.drive1
            pf.drive2
            pf.drive3
	    pf.cluster
            pf.nice

        In addition, you can use the parameter pf.disable to disable
        the driver entirely.

*/

/* Changes:

	1.01	GRG 1998.05.03  Changes for SMP.  Eliminate sti().
				Fix for drives that don't clear STAT_ERR
			        until after next CDB delivered.
				Small change in pf_completion to round
				up transfer size.
	1.02    GRG 1998.06.16  Eliminated an Ugh
	1.03    GRG 1998.08.16  Use HZ in loop timings, extra debugging
	1.04    GRG 1998.09.24  Added jumbo support

*/

#define PF_VERSION      "1.04"
#define PF_MAJOR	47
#define PF_NAME		"pf"
#define PF_UNITS	4

#include <linux/types.h>

/* Here are things one can override from the insmod command.
   Most are autoprobed by paride unless set here.  Verbose is off
   by default.

*/

static bool verbose = 0;
static int major = PF_MAJOR;
static char *name = PF_NAME;
static int cluster = 64;
static int nice = 0;
static int disable = 0;

static int drive0[7] = { 0, 0, 0, -1, -1, -1, -1 };
static int drive1[7] = { 0, 0, 0, -1, -1, -1, -1 };
static int drive2[7] = { 0, 0, 0, -1, -1, -1, -1 };
static int drive3[7] = { 0, 0, 0, -1, -1, -1, -1 };

static int (*drives[4])[7] = {&drive0, &drive1, &drive2, &drive3};
static int pf_drive_count;

enum {D_PRT, D_PRO, D_UNI, D_MOD, D_SLV, D_LUN, D_DLY};

/* end of parameters */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/hdreg.h>
#include <linux/cdrom.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>

static DEFINE_MUTEX(pf_mutex);
static DEFINE_SPINLOCK(pf_spin_lock);

module_param(verbose, bool, 0644);
module_param(major, int, 0);
module_param(name, charp, 0);
module_param(cluster, int, 0);
module_param(nice, int, 0);
module_param_array(drive0, int, NULL, 0);
module_param_array(drive1, int, NULL, 0);
module_param_array(drive2, int, NULL, 0);
module_param_array(drive3, int, NULL, 0);

#include "paride.h"
#include "pseudo.h"

/* constants for faking geometry numbers */

#define PF_FD_MAX	8192	/* use FD geometry under this size */
#define PF_FD_HDS	2
#define PF_FD_SPT	18
#define PF_HD_HDS	64
#define PF_HD_SPT	32

#define PF_MAX_RETRIES  5
#define PF_TMO          800	/* interrupt timeout in jiffies */
#define PF_SPIN_DEL     50	/* spin delay in micro-seconds  */

#define PF_SPIN         (1000000*PF_TMO)/(HZ*PF_SPIN_DEL)

#define STAT_ERR        0x00001
#define STAT_INDEX      0x00002
#define STAT_ECC        0x00004
#define STAT_DRQ        0x00008
#define STAT_SEEK       0x00010
#define STAT_WRERR      0x00020
#define STAT_READY      0x00040
#define STAT_BUSY       0x00080

#define ATAPI_REQ_SENSE		0x03
#define ATAPI_LOCK		0x1e
#define ATAPI_DOOR		0x1b
#define ATAPI_MODE_SENSE	0x5a
#define ATAPI_CAPACITY		0x25
#define ATAPI_IDENTIFY		0x12
#define ATAPI_READ_10		0x28
#define ATAPI_WRITE_10		0x2a

static int pf_open(struct block_device *bdev, fmode_t mode);
static void do_pf_request(struct request_queue * q);
static int pf_ioctl(struct block_device *bdev, fmode_t mode,
		    unsigned int cmd, unsigned long arg);
static int pf_getgeo(struct block_device *bdev, struct hd_geometry *geo);

static void pf_release(struct gendisk *disk, fmode_t mode);

static int pf_detect(void);
static void do_pf_read(void);
static void do_pf_read_start(void);
static void do_pf_write(void);
static void do_pf_write_start(void);
static void do_pf_read_drq(void);
static void do_pf_write_done(void);

#define PF_NM           0
#define PF_RO           1
#define PF_RW           2

#define PF_NAMELEN      8

struct pf_unit {
	struct pi_adapter pia;	/* interface to paride layer */
	struct pi_adapter *pi;
	int removable;		/* removable media device  ?  */
	int media_status;	/* media present ?  WP ? */
	int drive;		/* drive */
	int lun;
	int access;		/* count of active opens ... */
	int present;		/* device present ? */
	char name[PF_NAMELEN];	/* pf0, pf1, ... */
	struct gendisk *disk;
};

static struct pf_unit units[PF_UNITS];

static int pf_identify(struct pf_unit *pf);
static void pf_lock(struct pf_unit *pf, int func);
static void pf_eject(struct pf_unit *pf);
static unsigned int pf_check_events(struct gendisk *disk,
				    unsigned int clearing);

static char pf_scratch[512];	/* scratch block buffer */

/* the variables below are used mainly in the I/O request engine, which
   processes only one request at a time.
*/

static int pf_retries = 0;	/* i/o error retry count */
static int pf_busy = 0;		/* request being processed ? */
static struct request *pf_req;	/* current request */
static int pf_block;		/* address of next requested block */
static int pf_count;		/* number of blocks still to do */
static int pf_run;		/* sectors in current cluster */
static int pf_cmd;		/* current command READ/WRITE */
static struct pf_unit *pf_current;/* unit of current request */
static int pf_mask;		/* stopper for pseudo-int */
static char *pf_buf;		/* buffer for request in progress */
static void *par_drv;		/* reference of parport driver */

/* kernel glue structures */

static const struct block_device_operations pf_fops = {
	.owner		= THIS_MODULE,
	.open		= pf_open,
	.release	= pf_release,
	.ioctl		= pf_ioctl,
	.getgeo		= pf_getgeo,
	.check_events	= pf_check_events,
};

static void __init pf_init_units(void)
{
	struct pf_unit *pf;
	int unit;

	pf_drive_count = 0;
	for (unit = 0, pf = units; unit < PF_UNITS; unit++, pf++) {
		struct gendisk *disk = alloc_disk(1);
		if (!disk)
			continue;
		pf->disk = disk;
		pf->pi = &pf->pia;
		pf->media_status = PF_NM;
		pf->drive = (*drives[unit])[D_SLV];
		pf->lun = (*drives[unit])[D_LUN];
		snprintf(pf->name, PF_NAMELEN, "%s%d", name, unit);
		disk->major = major;
		disk->first_minor = unit;
		strcpy(disk->disk_name, pf->name);
		disk->fops = &pf_fops;
		if (!(*drives[unit])[D_PRT])
			pf_drive_count++;
	}
}

static int pf_open(struct block_device *bdev, fmode_t mode)
{
	struct pf_unit *pf = bdev->bd_disk->private_data;
	int ret;

	mutex_lock(&pf_mutex);
	pf_identify(pf);

	ret = -ENODEV;
	if (pf->media_status == PF_NM)
		goto out;

	ret = -EROFS;
	if ((pf->media_status == PF_RO) && (mode & FMODE_WRITE))
		goto out;

	ret = 0;
	pf->access++;
	if (pf->removable)
		pf_lock(pf, 1);
out:
	mutex_unlock(&pf_mutex);
	return ret;
}

static int pf_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	struct pf_unit *pf = bdev->bd_disk->private_data;
	sector_t capacity = get_capacity(pf->disk);

	if (capacity < PF_FD_MAX) {
		geo->cylinders = sector_div(capacity, PF_FD_HDS * PF_FD_SPT);
		geo->heads = PF_FD_HDS;
		geo->sectors = PF_FD_SPT;
	} else {
		geo->cylinders = sector_div(capacity, PF_HD_HDS * PF_HD_SPT);
		geo->heads = PF_HD_HDS;
		geo->sectors = PF_HD_SPT;
	}

	return 0;
}

static int pf_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	struct pf_unit *pf = bdev->bd_disk->private_data;

	if (cmd != CDROMEJECT)
		return -EINVAL;

	if (pf->access != 1)
		return -EBUSY;
	mutex_lock(&pf_mutex);
	pf_eject(pf);
	mutex_unlock(&pf_mutex);

	return 0;
}

static void pf_release(struct gendisk *disk, fmode_t mode)
{
	struct pf_unit *pf = disk->private_data;

	mutex_lock(&pf_mutex);
	if (pf->access <= 0) {
		mutex_unlock(&pf_mutex);
		WARN_ON(1);
		return;
	}

	pf->access--;

	if (!pf->access && pf->removable)
		pf_lock(pf, 0);

	mutex_unlock(&pf_mutex);
}

static unsigned int pf_check_events(struct gendisk *disk, unsigned int clearing)
{
	return DISK_EVENT_MEDIA_CHANGE;
}

static inline int status_reg(struct pf_unit *pf)
{
	return pi_read_regr(pf->pi, 1, 6);
}

static inline int read_reg(struct pf_unit *pf, int reg)
{
	return pi_read_regr(pf->pi, 0, reg);
}

static inline void write_reg(struct pf_unit *pf, int reg, int val)
{
	pi_write_regr(pf->pi, 0, reg, val);
}

static int pf_wait(struct pf_unit *pf, int go, int stop, char *fun, char *msg)
{
	int j, r, e, s, p;

	j = 0;
	while ((((r = status_reg(pf)) & go) || (stop && (!(r & stop))))
	       && (j++ < PF_SPIN))
		udelay(PF_SPIN_DEL);

	if ((r & (STAT_ERR & stop)) || (j > PF_SPIN)) {
		s = read_reg(pf, 7);
		e = read_reg(pf, 1);
		p = read_reg(pf, 2);
		if (j > PF_SPIN)
			e |= 0x100;
		if (fun)
			printk("%s: %s %s: alt=0x%x stat=0x%x err=0x%x"
			       " loop=%d phase=%d\n",
			       pf->name, fun, msg, r, s, e, j, p);
		return (e << 8) + s;
	}
	return 0;
}

static int pf_command(struct pf_unit *pf, char *cmd, int dlen, char *fun)
{
	pi_connect(pf->pi);

	write_reg(pf, 6, 0xa0+0x10*pf->drive);

	if (pf_wait(pf, STAT_BUSY | STAT_DRQ, 0, fun, "before command")) {
		pi_disconnect(pf->pi);
		return -1;
	}

	write_reg(pf, 4, dlen % 256);
	write_reg(pf, 5, dlen / 256);
	write_reg(pf, 7, 0xa0);	/* ATAPI packet command */

	if (pf_wait(pf, STAT_BUSY, STAT_DRQ, fun, "command DRQ")) {
		pi_disconnect(pf->pi);
		return -1;
	}

	if (read_reg(pf, 2) != 1) {
		printk("%s: %s: command phase error\n", pf->name, fun);
		pi_disconnect(pf->pi);
		return -1;
	}

	pi_write_block(pf->pi, cmd, 12);

	return 0;
}

static int pf_completion(struct pf_unit *pf, char *buf, char *fun)
{
	int r, s, n;

	r = pf_wait(pf, STAT_BUSY, STAT_DRQ | STAT_READY | STAT_ERR,
		    fun, "completion");

	if ((read_reg(pf, 2) & 2) && (read_reg(pf, 7) & STAT_DRQ)) {
		n = (((read_reg(pf, 4) + 256 * read_reg(pf, 5)) +
		      3) & 0xfffc);
		pi_read_block(pf->pi, buf, n);
	}

	s = pf_wait(pf, STAT_BUSY, STAT_READY | STAT_ERR, fun, "data done");

	pi_disconnect(pf->pi);

	return (r ? r : s);
}

static void pf_req_sense(struct pf_unit *pf, int quiet)
{
	char rs_cmd[12] =
	    { ATAPI_REQ_SENSE, pf->lun << 5, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0 };
	char buf[16];
	int r;

	r = pf_command(pf, rs_cmd, 16, "Request sense");
	mdelay(1);
	if (!r)
		pf_completion(pf, buf, "Request sense");

	if ((!r) && (!quiet))
		printk("%s: Sense key: %x, ASC: %x, ASQ: %x\n",
		       pf->name, buf[2] & 0xf, buf[12], buf[13]);
}

static int pf_atapi(struct pf_unit *pf, char *cmd, int dlen, char *buf, char *fun)
{
	int r;

	r = pf_command(pf, cmd, dlen, fun);
	mdelay(1);
	if (!r)
		r = pf_completion(pf, buf, fun);
	if (r)
		pf_req_sense(pf, !fun);

	return r;
}

static void pf_lock(struct pf_unit *pf, int func)
{
	char lo_cmd[12] = { ATAPI_LOCK, pf->lun << 5, 0, 0, func, 0, 0, 0, 0, 0, 0, 0 };

	pf_atapi(pf, lo_cmd, 0, pf_scratch, func ? "lock" : "unlock");
}

static void pf_eject(struct pf_unit *pf)
{
	char ej_cmd[12] = { ATAPI_DOOR, pf->lun << 5, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0 };

	pf_lock(pf, 0);
	pf_atapi(pf, ej_cmd, 0, pf_scratch, "eject");
}

#define PF_RESET_TMO   30	/* in tenths of a second */

static void pf_sleep(int cs)
{
	schedule_timeout_interruptible(cs);
}

/* the ATAPI standard actually specifies the contents of all 7 registers
   after a reset, but the specification is ambiguous concerning the last
   two bytes, and different drives interpret the standard differently.
 */

static int pf_reset(struct pf_unit *pf)
{
	int i, k, flg;
	int expect[5] = { 1, 1, 1, 0x14, 0xeb };

	pi_connect(pf->pi);
	write_reg(pf, 6, 0xa0+0x10*pf->drive);
	write_reg(pf, 7, 8);

	pf_sleep(20 * HZ / 1000);

	k = 0;
	while ((k++ < PF_RESET_TMO) && (status_reg(pf) & STAT_BUSY))
		pf_sleep(HZ / 10);

	flg = 1;
	for (i = 0; i < 5; i++)
		flg &= (read_reg(pf, i + 1) == expect[i]);

	if (verbose) {
		printk("%s: Reset (%d) signature = ", pf->name, k);
		for (i = 0; i < 5; i++)
			printk("%3x", read_reg(pf, i + 1));
		if (!flg)
			printk(" (incorrect)");
		printk("\n");
	}

	pi_disconnect(pf->pi);
	return flg - 1;
}

static void pf_mode_sense(struct pf_unit *pf)
{
	char ms_cmd[12] =
	    { ATAPI_MODE_SENSE, pf->lun << 5, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0 };
	char buf[8];

	pf_atapi(pf, ms_cmd, 8, buf, "mode sense");
	pf->media_status = PF_RW;
	if (buf[3] & 0x80)
		pf->media_status = PF_RO;
}

static void xs(char *buf, char *targ, int offs, int len)
{
	int j, k, l;

	j = 0;
	l = 0;
	for (k = 0; k < len; k++)
		if ((buf[k + offs] != 0x20) || (buf[k + offs] != l))
			l = targ[j++] = buf[k + offs];
	if (l == 0x20)
		j--;
	targ[j] = 0;
}

static int xl(char *buf, int offs)
{
	int v, k;

	v = 0;
	for (k = 0; k < 4; k++)
		v = v * 256 + (buf[k + offs] & 0xff);
	return v;
}

static void pf_get_capacity(struct pf_unit *pf)
{
	char rc_cmd[12] = { ATAPI_CAPACITY, pf->lun << 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	char buf[8];
	int bs;

	if (pf_atapi(pf, rc_cmd, 8, buf, "get capacity")) {
		pf->media_status = PF_NM;
		return;
	}
	set_capacity(pf->disk, xl(buf, 0) + 1);
	bs = xl(buf, 4);
	if (bs != 512) {
		set_capacity(pf->disk, 0);
		if (verbose)
			printk("%s: Drive %d, LUN %d,"
			       " unsupported block size %d\n",
			       pf->name, pf->drive, pf->lun, bs);
	}
}

static int pf_identify(struct pf_unit *pf)
{
	int dt, s;
	char *ms[2] = { "master", "slave" };
	char mf[10], id[18];
	char id_cmd[12] =
	    { ATAPI_IDENTIFY, pf->lun << 5, 0, 0, 36, 0, 0, 0, 0, 0, 0, 0 };
	char buf[36];

	s = pf_atapi(pf, id_cmd, 36, buf, "identify");
	if (s)
		return -1;

	dt = buf[0] & 0x1f;
	if ((dt != 0) && (dt != 7)) {
		if (verbose)
			printk("%s: Drive %d, LUN %d, unsupported type %d\n",
			       pf->name, pf->drive, pf->lun, dt);
		return -1;
	}

	xs(buf, mf, 8, 8);
	xs(buf, id, 16, 16);

	pf->removable = (buf[1] & 0x80);

	pf_mode_sense(pf);
	pf_mode_sense(pf);
	pf_mode_sense(pf);

	pf_get_capacity(pf);

	printk("%s: %s %s, %s LUN %d, type %d",
	       pf->name, mf, id, ms[pf->drive], pf->lun, dt);
	if (pf->removable)
		printk(", removable");
	if (pf->media_status == PF_NM)
		printk(", no media\n");
	else {
		if (pf->media_status == PF_RO)
			printk(", RO");
		printk(", %llu blocks\n",
			(unsigned long long)get_capacity(pf->disk));
	}
	return 0;
}

/*	returns  0, with id set if drive is detected
	        -1, if drive detection failed
*/
static int pf_probe(struct pf_unit *pf)
{
	if (pf->drive == -1) {
		for (pf->drive = 0; pf->drive <= 1; pf->drive++)
			if (!pf_reset(pf)) {
				if (pf->lun != -1)
					return pf_identify(pf);
				else
					for (pf->lun = 0; pf->lun < 8; pf->lun++)
						if (!pf_identify(pf))
							return 0;
			}
	} else {
		if (pf_reset(pf))
			return -1;
		if (pf->lun != -1)
			return pf_identify(pf);
		for (pf->lun = 0; pf->lun < 8; pf->lun++)
			if (!pf_identify(pf))
				return 0;
	}
	return -1;
}

static int pf_detect(void)
{
	struct pf_unit *pf = units;
	int k, unit;

	printk("%s: %s version %s, major %d, cluster %d, nice %d\n",
	       name, name, PF_VERSION, major, cluster, nice);

	par_drv = pi_register_driver(name);
	if (!par_drv) {
		pr_err("failed to register %s driver\n", name);
		return -1;
	}
	k = 0;
	if (pf_drive_count == 0) {
		if (pi_init(pf->pi, 1, -1, -1, -1, -1, -1, pf_scratch, PI_PF,
			    verbose, pf->name)) {
			if (!pf_probe(pf) && pf->disk) {
				pf->present = 1;
				k++;
			} else
				pi_release(pf->pi);
		}

	} else
		for (unit = 0; unit < PF_UNITS; unit++, pf++) {
			int *conf = *drives[unit];
			if (!conf[D_PRT])
				continue;
			if (pi_init(pf->pi, 0, conf[D_PRT], conf[D_MOD],
				    conf[D_UNI], conf[D_PRO], conf[D_DLY],
				    pf_scratch, PI_PF, verbose, pf->name)) {
				if (pf->disk && !pf_probe(pf)) {
					pf->present = 1;
					k++;
				} else
					pi_release(pf->pi);
			}
		}
	if (k)
		return 0;

	printk("%s: No ATAPI disk detected\n", name);
	for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++)
		put_disk(pf->disk);
	pi_unregister_driver(par_drv);
	return -1;
}

/* The i/o request engine */

static int pf_start(struct pf_unit *pf, int cmd, int b, int c)
{
	int i;
	char io_cmd[12] = { cmd, pf->lun << 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	for (i = 0; i < 4; i++) {
		io_cmd[5 - i] = b & 0xff;
		b = b >> 8;
	}

	io_cmd[8] = c & 0xff;
	io_cmd[7] = (c >> 8) & 0xff;

	i = pf_command(pf, io_cmd, c * 512, "start i/o");

	mdelay(1);

	return i;
}

static int pf_ready(void)
{
	return (((status_reg(pf_current) & (STAT_BUSY | pf_mask)) == pf_mask));
}

static struct request_queue *pf_queue;

static void pf_end_request(int err)
{
	if (pf_req && !__blk_end_request_cur(pf_req, err))
		pf_req = NULL;
}

static void do_pf_request(struct request_queue * q)
{
	if (pf_busy)
		return;
repeat:
	if (!pf_req) {
		pf_req = blk_fetch_request(q);
		if (!pf_req)
			return;
	}

	pf_current = pf_req->rq_disk->private_data;
	pf_block = blk_rq_pos(pf_req);
	pf_run = blk_rq_sectors(pf_req);
	pf_count = blk_rq_cur_sectors(pf_req);

	if (pf_block + pf_count > get_capacity(pf_req->rq_disk)) {
		pf_end_request(-EIO);
		goto repeat;
	}

	pf_cmd = rq_data_dir(pf_req);
	pf_buf = bio_data(pf_req->bio);
	pf_retries = 0;

	pf_busy = 1;
	if (pf_cmd == READ)
		pi_do_claimed(pf_current->pi, do_pf_read);
	else if (pf_cmd == WRITE)
		pi_do_claimed(pf_current->pi, do_pf_write);
	else {
		pf_busy = 0;
		pf_end_request(-EIO);
		goto repeat;
	}
}

static int pf_next_buf(void)
{
	unsigned long saved_flags;

	pf_count--;
	pf_run--;
	pf_buf += 512;
	pf_block++;
	if (!pf_run)
		return 1;
	if (!pf_count) {
		spin_lock_irqsave(&pf_spin_lock, saved_flags);
		pf_end_request(0);
		spin_unlock_irqrestore(&pf_spin_lock, saved_flags);
		if (!pf_req)
			return 1;
		pf_count = blk_rq_cur_sectors(pf_req);
		pf_buf = bio_data(pf_req->bio);
	}
	return 0;
}

static inline void next_request(int err)
{
	unsigned long saved_flags;

	spin_lock_irqsave(&pf_spin_lock, saved_flags);
	pf_end_request(err);
	pf_busy = 0;
	do_pf_request(pf_queue);
	spin_unlock_irqrestore(&pf_spin_lock, saved_flags);
}

/* detach from the calling context - in case the spinlock is held */
static void do_pf_read(void)
{
	ps_set_intr(do_pf_read_start, NULL, 0, nice);
}

static void do_pf_read_start(void)
{
	pf_busy = 1;

	if (pf_start(pf_current, ATAPI_READ_10, pf_block, pf_run)) {
		pi_disconnect(pf_current->pi);
		if (pf_retries < PF_MAX_RETRIES) {
			pf_retries++;
			pi_do_claimed(pf_current->pi, do_pf_read_start);
			return;
		}
		next_request(-EIO);
		return;
	}
	pf_mask = STAT_DRQ;
	ps_set_intr(do_pf_read_drq, pf_ready, PF_TMO, nice);
}

static void do_pf_read_drq(void)
{
	while (1) {
		if (pf_wait(pf_current, STAT_BUSY, STAT_DRQ | STAT_ERR,
			    "read block", "completion") & STAT_ERR) {
			pi_disconnect(pf_current->pi);
			if (pf_retries < PF_MAX_RETRIES) {
				pf_req_sense(pf_current, 0);
				pf_retries++;
				pi_do_claimed(pf_current->pi, do_pf_read_start);
				return;
			}
			next_request(-EIO);
			return;
		}
		pi_read_block(pf_current->pi, pf_buf, 512);
		if (pf_next_buf())
			break;
	}
	pi_disconnect(pf_current->pi);
	next_request(0);
}

static void do_pf_write(void)
{
	ps_set_intr(do_pf_write_start, NULL, 0, nice);
}

static void do_pf_write_start(void)
{
	pf_busy = 1;

	if (pf_start(pf_current, ATAPI_WRITE_10, pf_block, pf_run)) {
		pi_disconnect(pf_current->pi);
		if (pf_retries < PF_MAX_RETRIES) {
			pf_retries++;
			pi_do_claimed(pf_current->pi, do_pf_write_start);
			return;
		}
		next_request(-EIO);
		return;
	}

	while (1) {
		if (pf_wait(pf_current, STAT_BUSY, STAT_DRQ | STAT_ERR,
			    "write block", "data wait") & STAT_ERR) {
			pi_disconnect(pf_current->pi);
			if (pf_retries < PF_MAX_RETRIES) {
				pf_retries++;
				pi_do_claimed(pf_current->pi, do_pf_write_start);
				return;
			}
			next_request(-EIO);
			return;
		}
		pi_write_block(pf_current->pi, pf_buf, 512);
		if (pf_next_buf())
			break;
	}
	pf_mask = 0;
	ps_set_intr(do_pf_write_done, pf_ready, PF_TMO, nice);
}

static void do_pf_write_done(void)
{
	if (pf_wait(pf_current, STAT_BUSY, 0, "write block", "done") & STAT_ERR) {
		pi_disconnect(pf_current->pi);
		if (pf_retries < PF_MAX_RETRIES) {
			pf_retries++;
			pi_do_claimed(pf_current->pi, do_pf_write_start);
			return;
		}
		next_request(-EIO);
		return;
	}
	pi_disconnect(pf_current->pi);
	next_request(0);
}

static int __init pf_init(void)
{				/* preliminary initialisation */
	struct pf_unit *pf;
	int unit;

	if (disable)
		return -EINVAL;

	pf_init_units();

	if (pf_detect())
		return -ENODEV;
	pf_busy = 0;

	if (register_blkdev(major, name)) {
		for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++)
			put_disk(pf->disk);
		return -EBUSY;
	}
	pf_queue = blk_init_queue(do_pf_request, &pf_spin_lock);
	if (!pf_queue) {
		unregister_blkdev(major, name);
		for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++)
			put_disk(pf->disk);
		return -ENOMEM;
	}

	blk_queue_max_segments(pf_queue, cluster);

	for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++) {
		struct gendisk *disk = pf->disk;

		if (!pf->present)
			continue;
		disk->private_data = pf;
		disk->queue = pf_queue;
		add_disk(disk);
	}
	return 0;
}

static void __exit pf_exit(void)
{
	struct pf_unit *pf;
	int unit;
	unregister_blkdev(major, name);
	for (pf = units, unit = 0; unit < PF_UNITS; pf++, unit++) {
		if (!pf->present)
			continue;
		del_gendisk(pf->disk);
		put_disk(pf->disk);
		pi_release(pf->pi);
	}
	blk_cleanup_queue(pf_queue);
}

MODULE_LICENSE("GPL");
module_init(pf_init)
module_exit(pf_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /* 
	pg.c    (c) 1998  Grant R. Guenther <grant@torque.net>
			  Under the terms of the GNU General Public License.

	The pg driver provides a simple character device interface for
	sending ATAPI commands to a device.  With the exception of the
	ATAPI reset operation, all operations are performed by a pair
	of read and write operations to the appropriate /dev/pgN device.
	A write operation delivers a command and any outbound data in
	a single buffer.  Normally, the write will succeed unless the
	device is offline or malfunctioning, or there is already another
	command pending.  If the write succeeds, it should be followed
	immediately by a read operation, to obtain any returned data and
	status information.  A read will fail if there is no operation
	in progress.

	As a special case, the device can be reset with a write operation,
	and in this case, no following read is expected, or permitted.

	There are no ioctl() operations.  Any single operation
	may transfer at most PG_MAX_DATA bytes.  Note that the driver must
	copy the data through an internal buffer.  In keeping with all
	current ATAPI devices, command packets are assumed to be exactly
	12 bytes in length.

	To permit future changes to this interface, the headers in the
	read and write buffers contain a single character "magic" flag.
	Currently this flag must be the character "P".

	By default, the driver will autoprobe for a single parallel
	port ATAPI device, but if their individual parameters are
	specified, the driver can handle up to 4 devices.

	To use this device, you must have the following device 
	special files defined:

		/dev/pg0 c 97 0
		/dev/pg1 c 97 1
		/dev/pg2 c 97 2
		/dev/pg3 c 97 3

	(You'll need to change the 97 to something else if you use
	the 'major' parameter to install the driver on a different
	major number.)

	The behaviour of the pg driver can be altered by setting
	some parameters from the insmod command line.  The following
	parameters are adjustable:

	    drive0      These four arguments can be arrays of       
	    drive1      1-6 integers as follows:
	    drive2
	    drive3      <prt>,<pro>,<uni>,<mod>,<slv>,<dly>

			Where,

		<prt>   is the base of the parallel port address for
			the corresponding drive.  (required)

		<pro>   is the protocol number for the adapter that
			supports this drive.  These numbers are
			logged by 'paride' when the protocol modules
			are initialised.  (0 if not given)

		<uni>   for those adapters that support chained
			devices, this is the unit selector for the
			chain of devices on the given port.  It should
			be zero for devices that don't support chaining.
			(0 if not given)

		<mod>   this can be -1 to choose the best mode, or one
			of the mode numbers supported by the adapter.
			(-1 if not given)

		<slv>   ATAPI devices can be jumpered to master or slave.
			Set this to 0 to choose the master drive, 1 to
			choose the slave, -1 (the default) to choose the
			first drive found.

		<dly>   some parallel ports require the driver to 
			go more slowly.  -1 sets a default value that
			should work with the chosen protocol.  Otherwise,
			set this to a small integer, the larger it is
			the slower the port i/o.  In some cases, setting
			this to zero will speed up the device. (default -1)

	    major	You may use this parameter to overide the
			default major number (97) that this driver
			will use.  Be sure to change the device
			name as well.

	    name	This parameter is a character string that
			contains the name the kernel will use for this
			device (in /proc output, for instance).
			(default "pg").

	    verbose     This parameter controls the amount of logging
			that is done by the driver.  Set it to 0 for 
			quiet operation, to 1 to enable progress
			messages while the driver probes for devices,
			or to 2 for full debug logging.  (default 0)

	If this driver is built into the kernel, you can use 
	the following command line parameters, with the same values
	as the corresponding module parameters listed above:

	    pg.drive0
	    pg.drive1
	    pg.drive2
	    pg.drive3

	In addition, you can use the parameter pg.disable to disable
	the driver entirely.

*/

/* Changes:

	1.01	GRG 1998.06.16	Bug fixes
	1.02    GRG 1998.09.24  Added jumbo support

*/

#define PG_VERSION      "1.02"
#define PG_MAJOR	97
#define PG_NAME		"pg"
#define PG_UNITS	4

#ifndef PI_PG
#define PI_PG	4
#endif

#include <linux/types.h>
/* Here are things one can override from the insmod command.
   Most are autoprobed by paride unless set here.  Verbose is 0
   by default.

*/

static int verbose;
static int major = PG_MAJOR;
static char *name = PG_NAME;
static int disable = 0;

static int drive0[6] = { 0, 0, 0, -1, -1, -1 };
static int drive1[6] = { 0, 0, 0, -1, -1, -1 };
static int drive2[6] = { 0, 0, 0, -1, -1, -1 };
static int drive3[6] = { 0, 0, 0, -1, -1, -1 };

static int (*drives[4])[6] = {&drive0, &drive1, &drive2, &drive3};
static int pg_drive_count;

enum {D_PRT, D_PRO, D_UNI, D_MOD, D_SLV, D_DLY};

/* end of parameters */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mtio.h>
#include <linux/pg.h>
#include <linux/device.h>
#include <linux/sched.h>	/* current, TASK_* */
#include <linux/mutex.h>
#include <linux/jiffies.h>

#include <asm/uaccess.h>

module_param(verbose, int, 0644);
module_param(major, int, 0);
module_param(name, charp, 0);
module_param_array(drive0, int, NULL, 0);
module_param_array(drive1, int, NULL, 0);
module_param_array(drive2, int, NULL, 0);
module_param_array(drive3, int, NULL, 0);

#include "paride.h"

#define PG_SPIN_DEL     50	/* spin delay in micro-seconds  */
#define PG_SPIN         200
#define PG_TMO		HZ
#define PG_RESET_TMO	10*HZ

#define STAT_ERR        0x01
#define STAT_INDEX      0x02
#define STAT_ECC        0x04
#define STAT_DRQ        0x08
#define STAT_SEEK       0x10
#define STAT_WRERR      0x20
#define STAT_READY      0x40
#define STAT_BUSY       0x80

#define ATAPI_IDENTIFY		0x12

static DEFINE_MUTEX(pg_mutex);
static int pg_open(struct inode *inode, struct file *file);
static int pg_release(struct inode *inode, struct file *file);
static ssize_t pg_read(struct file *filp, char __user *buf,
		       size_t count, loff_t * ppos);
static ssize_t pg_write(struct file *filp, const char __user *buf,
			size_t count, loff_t * ppos);
static int pg_detect(void);

#define PG_NAMELEN      8

struct pg {
	struct pi_adapter pia;	/* interface to paride layer */
	struct pi_adapter *pi;
	int busy;		/* write done, read expected */
	int start;		/* jiffies at command start */
	int dlen;		/* transfer size requested */
	unsigned long timeout;	/* timeout requested */
	int status;		/* last sense key */
	int drive;		/* drive */
	unsigned long access;	/* count of active opens ... */
	int present;		/* device present ? */
	char *bufptr;
	char name[PG_NAMELEN];	/* pg0, pg1, ... */
};

static struct pg devices[PG_UNITS];

static int pg_identify(struct pg *dev, int log);

static char pg_scratch[512];	/* scratch block buffer */

static struct class *pg_class;
static void *par_drv;		/* reference of parport driver */

/* kernel glue structures */

static const struct file_operations pg_fops = {
	.owner = THIS_MODULE,
	.read = pg_read,
	.write = pg_write,
	.open = pg_open,
	.release = pg_release,
	.llseek = noop_llseek,
};

static void pg_init_units(void)
{
	int unit;

	pg_drive_count = 0;
	for (unit = 0; unit < PG_UNITS; unit++) {
		int *parm = *drives[unit];
		struct pg *dev = &devices[unit];
		dev->pi = &dev->pia;
		clear_bit(0, &dev->access);
		dev->busy = 0;
		dev->present = 0;
		de