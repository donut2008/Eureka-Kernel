  It means that (4) was already not true.

(2) can become not true only when pd_lock is released by the thread in question.
	Indeed, pd_busy is reset only in the area and thread that resets
	it is holding pd_lock.	The only place within the area where we
	release pd_lock is in pd_next_buf() (called from within the area).
	But that code does not reset pd_busy, so pd_busy would have to be
	0 when pd_next_buf() had acquired pd_lock.  If it become 0 while
	we were acquiring the lock, (1) would be already false, since
	the thread that had reset it would be in the area simulateously.
	If it was 0 before we tried to acquire pd_lock, (2) would be
	already false.

For similar reasons, (3) can become not true only when ps_spinlock is released
by the thread in question.  However, all such places within the area are right
after resetting ps_tq_active to 0.

(4) is done the same way - all places where we release pi_spinlock within
the area are either after resetting ->claimed_cont to NULL while holding
pi_spinlock, or after not tocuhing ->claimed_cont since acquiring pi_spinlock
also in the area.  The only place where ->claimed_cont is made non-NULL is
in the area, under pi_spinlock and we do not release it until after leaving
the area.

QED.


Corollary 1: ps_tq_active can be killed.  Indeed, the only place where we
check its value is in ps_set_intr() and if it had been non-zero at that
point, we would have violated either (2.1) (if it was set while ps_set_intr()
was acquiring ps_spinlock) or (2.3) (if it was set when we started to
acquire ps_spinlock).

Corollary 2: ps_spinlock can be killed.  Indeed, Lemma 1 and Lemma 2 show
that the only possible contention is between scheduling ps_tq followed by
immediate release of spinlock and beginning of execution of ps_tq on
another CPU.

Corollary 3: assignment to pd_busy in do_pd_read_start() and do_pd_write_start()
can be killed.  Indeed, we are not holding pd_lock and thus pd_busy is already
1 here.

Corollary 4: in ps_tq_int() uses of con can be replaced with uses of
ps_continuation, since the latter is changed only from the area.
We don't need to reset it to NULL, since we are guaranteed that there
will be a call of ps_set_intr() before we look at ps_continuation again.
We can remove the check for ps_continuation being NULL for the same
reason - the value is guaranteed to be set by the last ps_set_intr() and
we never pass it NULL.  Assignements in the beginning of ps_set_intr()
can be taken to callers as long as they remain within the area.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /* 
        aten.c  (c) 1997-8  Grant R. Guenther <grant@torque.net>
                            Under the terms of the GNU General Public License.

	aten.c is a low-level protocol driver for the ATEN EH-100
	parallel port adapter.  The EH-100 supports 4-bit and 8-bit
        modes only.  There is also an EH-132 which supports EPP mode
        transfers.  The EH-132 is not yet supported.

*/

/* Changes:

	1.01	GRG 1998.05.05	init_proto, release_proto

*/

#define ATEN_VERSION      "1.01"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <asm/io.h>

#include "paride.h"

#define j44(a,b)                ((((a>>4)&0x0f)|(b&0xf0))^0x88)

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int  cont_map[2] = { 0x08, 0x20 };

static void  aten_write_regr( PIA *pi, int cont, int regr, int val)

{	int r;

	r = regr + cont_map[cont] + 0x80;

	w0(r); w2(0xe); w2(6); w0(val); w2(7); w2(6); w2(0xc);
}

static int aten_read_regr( PIA *pi, int cont, int regr )

{	int  a, b, r;

        r = regr + cont_map[cont] + 0x40;

	switch (pi->mode) {

        case 0: w0(r); w2(0xe); w2(6); 
		w2(7); w2(6); w2(0);
		a = r1(); w0(0x10); b = r1(); w2(0xc);
		return j44(a,b);

        case 1: r |= 0x10;
		w0(r); w2(0xe); w2(6); w0(0xff); 
		w2(0x27); w2(0x26); w2(0x20);
		a = r0();
		w2(0x26); w2(0xc);
		return a;
	}
	return -1;
}

static void aten_read_block( PIA *pi, char * buf, int count )

{	int  k, a, b, c, d;

	switch (pi->mode) {

	case 0:	w0(0x48); w2(0xe); w2(6);
		for (k=0;k<count/2;k++) {
			w2(7); w2(6); w2(2);
			a = r1(); w0(0x58); b = r1();
			w2(0); d = r1(); w0(0x48); c = r1();
			buf[2*k] = j44(c,d);
			buf[2*k+1] = j44(a,b);
		}
		w2(0xc);
		break;

	case 1: w0(0x58); w2(0xe); w2(6);
		for (k=0;k<count/2;k++) {
			w2(0x27); w2(0x26); w2(0x22);
			a = r0(); w2(0x20); b = r0();
			buf[2*k] = b; buf[2*k+1] = a;
		}
		w2(0x26); w2(0xc);
		break;
	}
}

static void aten_write_block( PIA *pi, char * buf, int count )

{	int k;

	w0(0x88); w2(0xe); w2(6);
	for (k=0;k<count/2;k++) {
		w0(buf[2*k+1]); w2(0xe); w2(6);
		w0(buf[2*k]); w2(7); w2(6);
	}
	w2(0xc);
}

static void aten_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	w2(0xc);	
}

static void aten_disconnect ( PIA *pi )

{       w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void aten_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[2] = {"4-bit","8-bit"};

        printk("%s: aten %s, ATEN EH-100 at 0x%x, ",
                pi->device,ATEN_VERSION,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol aten = {
	.owner		= THIS_MODULE,
	.name		= "aten",
	.max_mode	= 2,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= aten_write_regr,
	.read_regr	= aten_read_regr,
	.write_block	= aten_write_block,
	.read_block	= aten_read_block,
	.connect	= aten_connect,
	.disconnect	= aten_disconnect,
	.log_adapter	= aten_log_adapter,
};

static int __init aten_init(void)
{
	return paride_register(&aten);
}

static void __exit aten_exit(void)
{
	paride_unregister( &aten );
}

MODULE_LICENSE("GPL");
module_init(aten_init)
module_exit(aten_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 /* 
	bpck.c	(c) 1996-8  Grant R. Guenther <grant@torque.net>
		            Under the terms of the GNU General Public License.

	bpck.c is a low-level protocol driver for the MicroSolutions 
	"backpack" parallel port IDE adapter.  

*/

/* Changes:

	1.01	GRG 1998.05.05 init_proto, release_proto, pi->delay 
	1.02    GRG 1998.08.15 default pi->delay returned to 4

*/

#define	BPCK_VERSION	"1.02" 

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#undef r2
#undef w2

#define PC			pi->private
#define r2()			(PC=(in_p(2) & 0xff))
#define w2(byte)  		{out_p(2,byte); PC = byte;}
#define t2(pat)   		{PC ^= pat; out_p(2,PC);}
#define e2()			{PC &= 0xfe; out_p(2,PC);}
#define o2()			{PC |= 1; out_p(2,PC);}

#define j44(l,h)     (((l>>3)&0x7)|((l>>4)&0x8)|((h<<1)&0x70)|(h&0x80))

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
   cont = 2 - use internal bpck register addressing
*/

static int  cont_map[3] = { 0x40, 0x48, 0 };

static int bpck_read_regr( PIA *pi, int cont, int regr )

{       int r, l, h;

	r = regr + cont_map[cont];

	switch (pi->mode) {

	case 0: w0(r & 0xf); w0(r); t2(2); t2(4);
	        l = r1();
        	t2(4);
        	h = r1();
        	return j44(l,h);

	case 1: w0(r & 0xf); w0(r); t2(2);
	        e2(); t2(0x20);
		t2(4); h = r0();
	        t2(1); t2(0x20);
	        return h;

	case 2:
	case 3:
	case 4: w0(r); w2(9); w2(0); w2(0x20);
		h = r4();
		w2(0);
		return h;

	}
	return -1;
}	

static void bpck_write_regr( PIA *pi, int cont, int regr, int val )

{	int	r;

        r = regr + cont_map[cont];

	switch (pi->mode) {

	case 0:
	case 1: w0(r);
		t2(2);
		w0(val);
		o2(); t2(4); t2(1);
		break;

	case 2:
	case 3:
	case 4: w0(r); w2(9); w2(0);
		w0(val); w2(1); w2(3); w2(0);
		break;

	}
}

/* These macros access the bpck registers in native addressing */

#define WR(r,v)		bpck_write_regr(pi,2,r,v)
#define RR(r)		(bpck_read_regr(pi,2,r))

static void bpck_write_block( PIA *pi, char * buf, int count )

{	int i;

	switch (pi->mode) {

	case 0: WR(4,0x40);
		w0(0x40); t2(2); t2(1);
		for (i=0;i<count;i++) { w0(buf[i]); t2(4); }
		WR(4,0);
		break;

	case 1: WR(4,0x50);
                w0(0x40); t2(2); t2(1);
                for (i=0;i<count;i++) { w0(buf[i]); t2(4); }
                WR(4,0x10);
		break;

	case 2: WR(4,0x48);
		w0(0x40); w2(9); w2(0); w2(1);
		for (i=0;i<count;i++) w4(buf[i]);
		w2(0);
		WR(4,8);
		break;

        case 3: WR(4,0x48);
                w0(0x40); w2(9); w2(0); w2(1);
                for (i=0;i<count/2;i++) w4w(((u16 *)buf)[i]);
                w2(0);
                WR(4,8);
                break;
 
        case 4: WR(4,0x48);
                w0(0x40); w2(9); w2(0); w2(1);
                for (i=0;i<count/4;i++) w4l(((u32 *)buf)[i]);
                w2(0);
                WR(4,8);
                break;
 	}
}

static void bpck_read_block( PIA *pi, char * buf, int count )

{	int i, l, h;

	switch (pi->mode) {

      	case 0: WR(4,0x40);
		w0(0x40); t2(2);
		for (i=0;i<count;i++) {
		    t2(4); l = r1();
		    t2(4); h = r1();
		    buf[i] = j44(l,h);
		}
		WR(4,0);
		break;

	case 1: WR(4,0x50);
		w0(0x40); t2(2); t2(0x20);
      	        for(i=0;i<count;i++) { t2(4); buf[i] = r0(); }
	        t2(1); t2(0x20);
	        WR(4,0x10);
		break;

	case 2: WR(4,0x48);
		w0(0x40); w2(9); w2(0); w2(0x20);
		for (i=0;i<count;i++) buf[i] = r4();
		w2(0);
		WR(4,8);
		break;

        case 3: WR(4,0x48);
                w0(0x40); w2(9); w2(0); w2(0x20);
                for (i=0;i<count/2;i++) ((u16 *)buf)[i] = r4w();
                w2(0);
                WR(4,8);
                break;

        case 4: WR(4,0x48);
                w0(0x40); w2(9); w2(0); w2(0x20);
                for (i=0;i<count/4;i++) ((u32 *)buf)[i] = r4l();
                w2(0);
                WR(4,8);
                break;

	}
}

static int bpck_probe_unit ( PIA *pi )

{	int o1, o0, f7, id;
	int t, s;

	id = pi->unit;
	s = 0;
	w2(4); w2(0xe); r2(); t2(2); 
	o1 = r1()&0xf8;
	o0 = r0();
	w0(255-id); w2(4); w0(id);
	t2(8); t2(8); t2(8);
	t2(2); t = r1()&0xf8;
	f7 = ((id % 8) == 7);
	if ((f7) || (t != o1)) { t2(2); s = r1()&0xf8; }
	if ((t == o1) && ((!f7) || (s == o1)))  {
		w2(0x4c); w0(o0);
		return 0;	
	}
	t2(8); w0(0); t2(2); w2(0x4c); w0(o0);
	return 1;
}
	
static void bpck_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
	w0(0xff-pi->unit); w2(4); w0(pi->unit);
	t2(8); t2(8); t2(8); 
	t2(2); t2(2);
	
	switch (pi->mode) {

	case 0: t2(8); WR(4,0);
		break;

	case 1: t2(8); WR(4,0x10);
		break;

	case 2:
        case 3:
	case 4: w2(0); WR(4,8);
		break;

	}

	WR(5,8);

	if (pi->devtype == PI_PCD) {
		WR(0x46,0x10);		/* fiddle with ESS logic ??? */
		WR(0x4c,0x38);
		WR(0x4d,0x88);
		WR(0x46,0xa0);
		WR(0x41,0);
		WR(0x4e,8);
		}
}

static void bpck_disconnect ( PIA *pi )

{	w0(0); 
	if (pi->mode >= 2) { w2(9); w2(0); } else t2(2);
	w2(0x4c); w0(pi->saved_r0);
} 

static void bpck_force_spp ( PIA *pi )

/* This fakes the EPP protocol to turn off EPP ... */

{       pi->saved_r0 = r0();
        w0(0xff-pi->unit); w2(4); w0(pi->unit);
        t2(8); t2(8); t2(8); 
        t2(2); t2(2);

        w2(0); 
        w0(4); w2(9); w2(0); 
        w0(0); w2(1); w2(3); w2(0);     
        w0(0); w2(9); w2(0);
        w2(0x4c); w0(pi->saved_r0);
}

#define TEST_LEN  16

static int bpck_test_proto( PIA *pi, char * scratch, int verbose )

{	int i, e, l, h, om;
	char buf[TEST_LEN];

	bpck_force_spp(pi);

	switch (pi->mode) {

	case 0: bpck_connect(pi);
		WR(0x13,0x7f);
		w0(0x13); t2(2);
		for(i=0;i<TEST_LEN;i++) {
                    t2(4); l = r1();
                    t2(4); h = r1();
                    buf[i] = j44(l,h);
		}
		bpck_disconnect(pi);
		break;

        case 1: bpck_connect(pi);
		WR(0x13,0x7f);
                w0(0x13); t2(2); t2(0x20);
                for(i=0;i<TEST_LEN;i++) { t2(4); buf[i] = r0(); }
                t2(1); t2(0x20);
		bpck_disconnect(pi);
		break;

	case 2:
	case 3:
	case 4: om = pi->mode;
		pi->mode = 0;
		bpck_connect(pi);
		WR(7,3);
		WR(4,8);
		bpck_disconnect(pi);

		pi->mode = om;
		bpck_connect(pi);
		w0(0x13); w2(9); w2(1); w0(0); w2(3); w2(0); w2(0xe0);

		switch (pi->mode) {
		  case 2: for (i=0;i<TEST_LEN;i++) buf[i] = r4();
			  break;
		  case 3: for (i=0;i<TEST_LEN/2;i++) ((u16 *)buf)[i] = r4w();
                          break;
		  case 4: for (i=0;i<TEST_LEN/4;i++) ((u32 *)buf)[i] = r4l();
                          break;
		}

		w2(0);
		WR(7,0);
		bpck_disconnect(pi);

		break;

	}

	if (verbose) {
	    printk("%s: bpck: 0x%x unit %d mode %d: ",
		   pi->device,pi->port,pi->unit,pi->mode);
	    for (i=0;i<TEST_LEN;i++) printk("%3d",buf[i]);
	    printk("\n");
	}

	e = 0;
	for (i=0;i<TEST_LEN;i++) if (buf[i] != (i+1)) e++;
	return e;
}

static void bpck_read_eeprom ( PIA *pi, char * buf )

{       int i,j,k,n,p,v,f, om, od;

	bpck_force_spp(pi);

	om = pi->mode;  od = pi->delay;
	pi->mode = 0; pi->delay = 6;

	bpck_connect(pi);
	
	n = 0;
	WR(4,0);
	for (i=0;i<64;i++) {
	    WR(6,8);  
	    WR(6,0xc);
	    p = 0x100;
	    for (k=0;k<9;k++) {
		f = (((i + 0x180) & p) != 0) * 2;
		WR(6,f+0xc); 
		WR(6,f+0xd); 
		WR(6,f+0xc);
		p = (p >> 1);
	    }
	    for (j=0;j<2;j++) {
		v = 0;
		for (k=0;k<8;k++) {
		    WR(6,0xc); 
		    WR(6,0xd); 
		    WR(6,0xc); 
		    f = RR(0);
		    v = 2*v + (f == 0x84);
		}
		buf[2*i+1-j] = v;
	    }
	}
	WR(6,8);
	WR(6,0);
	WR(5,8);

	bpck_disconnect(pi);

        if (om >= 2) {
                bpck_connect(pi);
                WR(7,3);
                WR(4,8);
                bpck_disconnect(pi);
        }

	pi->mode = om; pi->delay = od;
}

static int bpck_test_port ( PIA *pi ) 	/* check for 8-bit port */

{	int	i, r, m;

	w2(0x2c); i = r0(); w0(255-i); r = r0(); w0(i);
	m = -1;
	if (r == i) m = 2;
	if (r == (255-i)) m = 0;

	w2(0xc); i = r0(); w0(255-i); r = r0(); w0(i);
	if (r != (255-i)) m = -1;
	
	if (m == 0) { w2(6); w2(0xc); r = r0(); w0(0xaa); w0(r); w0(0xaa); }
	if (m == 2) { w2(0x26); w2(0xc); }

	if (m == -1) return 0;
	return 5;
}

static void bpck_log_adapter( PIA *pi, char * scratch, int verbose )

{	char	*mode_string[5] = { "4-bit","8-bit","EPP-8",
				    "EPP-16","EPP-32" };

#ifdef DUMP_EEPROM
	int i;
#endif

	bpck_read_eeprom(pi,scratch);

#ifdef DUMP_EEPROM
	if (verbose) {
	   for(i=0;i<128;i++)
		if ((scratch[i] < ' ') || (scratch[i] > '~'))
		    scratch[i] = '.';
	   printk("%s: bpck EEPROM: %64.64s\n",pi->device,scratch);
	   printk("%s:              %64.64s\n",pi->device,&scratch[64]);
	}
#endif

	printk("%s: bpck %s, backpack %8.8s unit %d",
		pi->device,BPCK_VERSION,&scratch[110],pi->unit);
	printk(" at 0x%x, mode %d (%s), delay %d\n",pi->port,
		pi->mode,mode_string[pi->mode],pi->delay);
}

static struct pi_protocol bpck = {
	.owner		= THIS_MODULE,
	.name		= "bpck",
	.max_mode	= 5,
	.epp_first	= 2,
	.default_delay	= 4,
	.max_units	= 255,
	.write_regr	= bpck_write_regr,
	.read_regr	= bpck_read_regr,
	.write_block	= bpck_write_block,
	.read_block	= bpck_read_block,
	.connect	= bpck_connect,
	.disconnect	= bpck_disconnect,
	.test_port	= bpck_test_port,
	.probe_unit	= bpck_probe_unit,
	.test_proto	= bpck_test_proto,
	.log_adapter	= bpck_log_adapter,
};

static int __init bpck_init(void)
{
	return paride_register(&bpck);
}

static void __exit bpck_exit(void)
{
	paride_unregister(&bpck);
}

MODULE_LICENSE("GPL");
module_init(bpck_init)
module_exit(bpck_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*
	backpack.c (c) 2001 Micro Solutions Inc.
		Released under the terms of the GNU General Public license

	backpack.c is a low-level protocol driver for the Micro Solutions
		"BACKPACK" parallel port IDE adapter
		(Works on Series 6 drives)

	Written by: Ken Hahn     (linux-dev@micro-solutions.com)
	            Clive Turvey (linux-dev@micro-solutions.com)

*/

/*
   This is Ken's linux wrapper for the PPC library
   Version 1.0.0 is the backpack driver for which source is not available
   Version 2.0.0 is the first to have source released 
   Version 2.0.1 is the "Cox-ified" source code 
   Version 2.0.2 - fixed version string usage, and made ppc functions static 
*/


#define BACKPACK_VERSION "2.0.2"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <asm/io.h>
#include <linux/parport.h>

#include "ppc6lnx.c"
#include "paride.h"

/* PARAMETERS */
static bool verbose; /* set this to 1 to see debugging messages and whatnot */
 

#define PPCSTRUCT(pi) ((Interface *)(pi->private))

/****************************************************************/
/*
 ATAPI CDROM DRIVE REGISTERS
*/
#define ATAPI_DATA       0      /* data port                  */
#define ATAPI_ERROR      1      /* error register (read)      */
#define ATAPI_FEATURES   1      /* feature register (write)   */
#define ATAPI_INT_REASON 2      /* interrupt reason register  */
#define ATAPI_COUNT_LOW  4      /* byte count register (low)  */
#define ATAPI_COUNT_HIGH 5      /* byte count register (high) */
#define ATAPI_DRIVE_SEL  6      /* drive select register      */
#define ATAPI_STATUS     7      /* status port (read)         */
#define ATAPI_COMMAND    7      /* command port (write)       */
#define ATAPI_ALT_STATUS 0x0e /* alternate status reg (read) */
#define ATAPI_DEVICE_CONTROL 0x0e /* device control (write)   */
/****************************************************************/

static int bpck6_read_regr(PIA *pi, int cont, int reg)
{
	unsigned int out;

	/* check for bad settings */
	if (reg<0 || reg>7 || cont<0 || cont>2)
	{
		return(-1);
	}
	out=ppc6_rd_port(PPCSTRUCT(pi),cont?reg|8:reg);
	return(out);
}

static void bpck6_write_regr(PIA *pi, int cont, int reg, int val)
{
	/* check for bad settings */
	if (reg>=0 && reg<=7 && cont>=0 && cont<=1)
	{
		ppc6_wr_port(PPCSTRUCT(pi),cont?reg|8:reg,(u8)val);
	}
}

static void bpck6_write_block( PIA *pi, char * buf, int len )
{
	ppc6_wr_port16_blk(PPCSTRUCT(pi),ATAPI_DATA,buf,(u32)len>>1); 
}

static void bpck6_read_block( PIA *pi, char * buf, int len )
{
	ppc6_rd_port16_blk(PPCSTRUCT(pi),ATAPI_DATA,buf,(u32)len>>1);
}

static void bpck6_connect ( PIA *pi  )
{
	if(verbose)
	{
		printk(KERN_DEBUG "connect\n");
	}

	if(pi->mode >=2)
  	{
		PPCSTRUCT(pi)->mode=4+pi->mode-2;	
	}
	else if(pi->mode==1)
	{
		PPCSTRUCT(pi)->mode=3;	
	}
	else
	{
		PPCSTRUCT(pi)->mode=1;		
	}

	ppc6_open(PPCSTRUCT(pi));  
	ppc6_wr_extout(PPCSTRUCT(pi),0x3);
}

static void bpck6_disconnect ( PIA *pi )
{
	if(verbose)
	{
		printk("disconnect\n");
	}
	ppc6_wr_extout(PPCSTRUCT(pi),0x0);
	ppc6_close(PPCSTRUCT(pi));
}

static int bpck6_test_port ( PIA *pi )   /* check for 8-bit port */
{
	if(verbose)
	{
		printk(KERN_DEBUG "PARPORT indicates modes=%x for lp=0x%lx\n",
               		((struct pardevice*)(pi->pardev))->port->modes,
			((struct pardevice *)(pi->pardev))->port->base); 
	}

	/*copy over duplicate stuff.. initialize state info*/
	PPCSTRUCT(pi)->ppc_id=pi->unit;
	PPCSTRUCT(pi)->lpt_addr=pi->port;

	/* look at the parport device to see if what modes we can use */
	if(((struct pardevice *)(pi->pardev))->port->modes & 
		(PARPORT_MODE_EPP)
          )
	{
		return 5; /* Can do EPP*/
	}
	else if(((struct pardevice *)(pi->pardev))->port->modes & 
			(PARPORT_MODE_TRISTATE)
               )
	{
		return 2;
	}
	else /*Just flat SPP*/
	{
		return 1;
	}
}

static int bpck6_probe_unit ( PIA *pi )
{
	int out;

	if(verbose)
	{
		printk(KERN_DEBUG "PROBE UNIT %x on port:%x\n",pi->unit,pi->port);
	}

	/*SET PPC UNIT NUMBER*/
	PPCSTRUCT(pi)->ppc_id=pi->unit;

	/*LOWER DOWN TO UNIDIRECTIONAL*/
	PPCSTRUCT(pi)->mode=1;		

	out=ppc6_open(PPCSTRUCT(pi));

	if(verbose)
	{
		printk(KERN_DEBUG "ppc_open returned %2x\n",out);
	}

  	if(out)
 	{
		ppc6_close(PPCSTRUCT(pi));
		if(verbose)
		{
			printk(KERN_DEBUG "leaving probe\n");
		}
               return(1);
	}
  	else
  	{
		if(verbose)
		{
			printk(KERN_DEBUG "Failed open\n");
		}
    		return(0);
  	}
}

static void bpck6_log_adapter( PIA *pi, char * scratch, int verbose )
{
	char *mode_string[5]=
		{"4-bit","8-bit","EPP-8","EPP-16","EPP-32"};

	printk("%s: BACKPACK Protocol Driver V"BACKPACK_VERSION"\n",pi->device);
	printk("%s: Copyright 2001 by Micro Solutions, Inc., DeKalb IL.\n",pi->device);
	printk("%s: BACKPACK %s, Micro Solutions BACKPACK Drive at 0x%x\n",
		pi->device,BACKPACK_VERSION,pi->port);
	printk("%s: Unit: %d Mode:%d (%s) Delay %d\n",pi->device,
		pi->unit,pi->mode,mode_string[pi->mode],pi->delay);
}

static int bpck6_init_proto(PIA *pi)
{
	Interface *p = kzalloc(sizeof(Interface), GFP_KERNEL);

	if (p) {
		pi->private = (unsigned long)p;
		return 0;
	}

	printk(KERN_ERR "%s: ERROR COULDN'T ALLOCATE MEMORY\n", pi->device); 
	return -1;
}

static void bpck6_release_proto(PIA *pi)
{
	kfree((void *)(pi->private)); 
}

static struct pi_protocol bpck6 = {
	.owner		= THIS_MODULE,
	.name		= "bpck6",
	.max_mode	= 5,
	.epp_first	= 2, /* 2-5 use epp (need 8 ports) */
	.max_units	= 255,
	.write_regr	= bpck6_write_regr,
	.read_regr	= bpck6_read_regr,
	.write_block	= bpck6_write_block,
	.read_block	= bpck6_read_block,
	.connect	= bpck6_connect,
	.disconnect	= bpck6_disconnect,
	.test_port	= bpck6_test_port,
	.probe_unit	= bpck6_probe_unit,
	.log_adapter	= bpck6_log_adapter,
	.init_proto	= bpck6_init_proto,
	.release_proto	= bpck6_release_proto,
};

static int __init bpck6_init(void)
{
	printk(KERN_INFO "bpck6: BACKPACK Protocol Driver V"BACKPACK_VERSION"\n");
	printk(KERN_INFO "bpck6: Copyright 2001 by Micro Solutions, Inc., DeKalb IL. USA\n");
	if(verbose)
		printk(KERN_DEBUG "bpck6: verbose debug enabled.\n");
	return paride_register(&bpck6);
}

static void __exit bpck6_exit(void)
{
	paride_unregister(&bpck6);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Micro Solutions Inc.");
MODULE_DESCRIPTION("BACKPACK Protocol module, compatible with PARIDE");
module_param(verbose, bool, 0644);
module_init(bpck6_init)
module_exit(bpck6_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /* 
        comm.c    (c) 1997-8  Grant R. Guenther <grant@torque.net>
                              Under the terms of the GNU General Public License.

	comm.c is a low-level protocol driver for some older models
	of the DataStor "Commuter" parallel to IDE adapter.  Some of
	the parallel port devices marketed by Arista currently
	use this adapter.
*/

/* Changes:

	1.01	GRG 1998.05.05  init_proto, release_proto

*/

#define COMM_VERSION      "1.01"

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
*/

#define j44(a,b)	(((a>>3)&0x0f)|((b<<1)&0xf0))

#define P1	w2(5);w2(0xd);w2(0xd);w2(5);w2(4);
#define P2	w2(5);w2(7);w2(7);w2(5);w2(4);

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int  cont_map[2] = { 0x08, 0x10 };

static int comm_read_regr( PIA *pi, int cont, int regr )

{       int     l, h, r;

        r = regr + cont_map[cont];

        switch (pi->mode)  {

        case 0: w0(r); P1; w0(0);
        	w2(6); l = r1(); w0(0x80); h = r1(); w2(4);
                return j44(l,h);

        case 1: w0(r+0x20); P1; 
        	w0(0); w2(0x26); h = r0(); w2(4);
                return h;

	case 2:
	case 3:
        case 4: w3(r+0x20); (void)r1();
        	w2(0x24); h = r4(); w2(4);
                return h;

        }
        return -1;
}       

static void comm_write_regr( PIA *pi, int cont, int regr, int val )

{       int  r;

        r = regr + cont_map[cont];

        switch (pi->mode)  {

        case 0:
        case 1: w0(r); P1; w0(val); P2;
		break;

	case 2:
	case 3:
        case 4: w3(r); (void)r1(); w4(val);
                break;
        }
}

static void comm_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
        w2(4); w0(0xff); w2(6);
        w2(4); w0(0xaa); w2(6);
        w2(4); w0(0x00); w2(6);
        w2(4); w0(0x87); w2(6);
        w2(4); w0(0xe0); w2(0xc); w2(0xc); w2(4);
}

static void comm_disconnect ( PIA *pi )

{       w2(0); w2(0); w2(0); w2(4); 
	w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void comm_read_block( PIA *pi, char * buf, int count )

{       int     i, l, h;

        switch (pi->mode) {
        
        case 0: w0(0x48); P1;
                for(i=0;i<count;i++) {
                        w0(0); w2(6); l = r1();
                        w0(0x80); h = r1(); w2(4);
                        buf[i] = j44(l,h);
                }
                break;

        case 1: w0(0x68); P1; w0(0);
                for(i=0;i<count;i++) {
                        w2(0x26); buf[i] = r0(); w2(0x24);
                }
		w2(4);
		break;
		
	case 2: w3(0x68); (void)r1(); w2(0x24);
		for (i=0;i<count;i++) buf[i] = r4();
		w2(4);
		break;

        case 3: w3(0x68); (void)r1(); w2(0x24);
                for (i=0;i<count/2;i++) ((u16 *)buf)[i] = r4w();
                w2(4);
                break;

        case 4: w3(0x68); (void)r1(); w2(0x24);
                for (i=0;i<count/4;i++) ((u32 *)buf)[i] = r4l();
                w2(4);
                break;
		
	}
}

/* NB: Watch out for the byte swapped writes ! */

static void comm_write_block( PIA *pi, char * buf, int count )

{       int	k;

        switch (pi->mode) {

        case 0:
        case 1: w0(0x68); P1;
        	for (k=0;k<count;k++) {
                        w2(5); w0(buf[k^1]); w2(7);
                }
                w2(5); w2(4);
                break;

        case 2: w3(0x48); (void)r1();
                for (k=0;k<count;k++) w4(buf[k^1]);
                break;

        case 3: w3(0x48); (void)r1();
                for (k=0;k<count/2;k++) w4w(pi_swab16(buf,k));
                break;

        case 4: w3(0x48); (void)r1();
                for (k=0;k<count/4;k++) w4l(pi_swab32(buf,k));
                break;


        }
}

static void comm_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[5] = {"4-bit","8-bit","EPP-8","EPP-16","EPP-32"};

        printk("%s: comm %s, DataStor Commuter at 0x%x, ",
                pi->device,COMM_VERSION,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol comm = {
	.owner		= THIS_MODULE,
	.name		= "comm",
	.max_mode	= 5,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= comm_write_regr,
	.read_regr	= comm_read_regr,
	.write_block	= comm_write_block,
	.read_block	= comm_read_block,
	.connect	= comm_connect,
	.disconnect	= comm_disconnect,
	.log_adapter	= comm_log_adapter,
};

static int __init comm_init(void)
{
	return paride_register(&comm);
}

static void __exit comm_exit(void)
{
	paride_unregister(&comm);
}

MODULE_LICENSE("GPL");
module_init(comm_init)
module_exit(comm_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /* 
        dstr.c    (c) 1997-8  Grant R. Guenther <grant@torque.net>
                              Under the terms of the GNU General Public License.

        dstr.c is a low-level protocol driver for the 
        DataStor EP2000 parallel to IDE adapter chip.

*/

/* Changes:

        1.01    GRG 1998.05.06 init_proto, release_proto

*/

#define DSTR_VERSION      "1.01"

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

#define j44(a,b)  (((a>>3)&0x07)|((~a>>4)&0x08)|((b<<1)&0x70)|((~b)&0x80))

#define P1	w2(5);w2(0xd);w2(5);w2(4);
#define P2	w2(5);w2(7);w2(5);w2(4);
#define P3      w2(6);w2(4);w2(6);w2(4);

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int  cont_map[2] = { 0x20, 0x40 };

static int dstr_read_regr( PIA *pi, int cont, int regr )

{       int     a, b, r;

        r = regr + cont_map[cont];

	w0(0x81); P1;
	if (pi->mode) { w0(0x11); } else { w0(1); }
	P2; w0(r); P1;

        switch (pi->mode)  {

        case 0: w2(6); a = r1(); w2(4); w2(6); b = r1(); w2(4);
                return j44(a,b);

        case 1: w0(0); w2(0x26); a = r0(); w2(4);
                return a;

	case 2:
	case 3:
        case 4: w2(0x24); a = r4(); w2(4);
                return a;

        }
        return -1;
}       

static void dstr_write_regr(  PIA *pi, int cont, int regr, int val )

{       int  r;

        r = regr + cont_map[cont];

	w0(0x81); P1; 
	if (pi->mode >= 2) { w0(0x11); } else { w0(1); }
	P2; w0(r); P1;
	
        switch (pi->mode)  {

        case 0:
        case 1: w0(val); w2(5); w2(7); w2(5); w2(4);
		break;

	case 2:
	case 3:
        case 4: w4(val); 
                break;
        }
}

#define  CCP(x)  w0(0xff);w2(0xc);w2(4);\
		 w0(0xaa);w0(0x55);w0(0);w0(0xff);w0(0x87);w0(0x78);\
		 w0(x);w2(5);w2(4);

static void dstr_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
        w2(4); CCP(0xe0); w0(0xff);
}

static void dstr_disconnect ( PIA *pi )

{       CCP(0x30);
        w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void dstr_read_block( PIA *pi, char * buf, int count )

{       int     k, a, b;

        w0(0x81); P1;
        if (pi->mode) { w0(0x19); } else { w0(9); }
	P2; w0(0x82); P1; P3; w0(0x20); P1;

        switch (pi->mode) {

        case 0: for (k=0;k<count;k++) {
                        w2(6); a = r1(); w2(4);
                        w2(6); b = r1(); w2(4);
                        buf[k] = j44(a,b);
                } 
                break;

        case 1: w0(0);
                for (k=0;k<count;k++) {
                        w2(0x26); buf[k] = r0(); w2(0x24);
                }
                w2(4);
                break;

        case 2: w2(0x24); 
                for (k=0;k<count;k++) buf[k] = r4();
                w2(4);
                break;

        case 3: w2(0x24); 
                for (k=0;k<count/2;k++) ((u16 *)buf)[k] = r4w();
                w2(4);
                break;

        case 4: w2(0x24); 
                for (k=0;k<count/4;k++) ((u32 *)buf)[k] = r4l();
                w2(4);
                break;

        }
}

static void dstr_write_block( PIA *pi, char * buf, int count )

{       int	k;

        w0(0x81); P1;
        if (pi->mode) { w0(0x19); } else { w0(9); }
        P2; w0(0x82); P1; P3; w0(0x20); P1;

        switch (pi->mode) {

        case 0:
        case 1: for (k=0;k<count;k++) {
                        w2(5); w0(buf[k]); w2(7);
                }
                w2(5); w2(4);
                break;

        case 2: w2(0xc5);
                for (k=0;k<count;k++) w4(buf[k]);
		w2(0xc4);
                break;

        case 3: w2(0xc5);
                for (k=0;k<count/2;k++) w4w(((u16 *)buf)[k]);
                w2(0xc4);
                break;

        case 4: w2(0xc5);
                for (k=0;k<count/4;k++) w4l(((u32 *)buf)[k]);
                w2(0xc4);
                break;

        }
}


static void dstr_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[5] = {"4-bit","8-bit","EPP-8",
				   "EPP-16","EPP-32"};

        printk("%s: dstr %s, DataStor EP2000 at 0x%x, ",
                pi->device,DSTR_VERSION,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol dstr = {
	.owner		= THIS_MODULE,
	.name		= "dstr",
	.max_mode	= 5,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= dstr_write_regr,
	.read_regr	= dstr_read_regr,
	.write_block	= dstr_write_block,
	.read_block	= dstr_read_block,
	.connect	= dstr_connect,
	.disconnect	= dstr_disconnect,
	.log_adapter	= dstr_log_adapter,
};

static int __init dstr_init(void)
{
	return paride_register(&dstr);
}

static void __exit dstr_exit(void)
{
	paride_unregister(&dstr);
}

MODULE_LICENSE("GPL");
module_init(dstr_init)
module_exit(dstr_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         /* 
        epat.c  (c) 1997-8  Grant R. Guenther <grant@torque.net>
                            Under the terms of the GNU General Public License.

	This is the low level protocol driver for the EPAT parallel
        to IDE adapter from Shuttle Technologies.  This adapter is
        used in many popular parallel port disk products such as the
        SyQuest EZ drives, the Avatar Shark and the Imation SuperDisk.
	
*/

/* Changes:

        1.01    GRG 1998.05.06 init_proto, release_proto
        1.02    Joshua b. Jore CPP(renamed), epat_connect, epat_disconnect

*/

#define EPAT_VERSION      "1.02"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#define j44(a,b)		(((a>>4)&0x0f)+(b&0xf0))
#define j53(a,b)		(((a>>3)&0x1f)+((b<<4)&0xe0))

static int epatc8;

module_param(epatc8, int, 0);
MODULE_PARM_DESC(epatc8, "support for the Shuttle EP1284 chip, "
	"used in any recent Imation SuperDisk (LS-120) drive.");

/* cont =  0   IDE register file
   cont =  1   IDE control registers
   cont =  2   internal EPAT registers
*/

static int cont_map[3] = { 0x18, 0x10, 0 };

static void epat_write_regr( PIA *pi, int cont, int regr, int val)

{	int r;

	r = regr + cont_map[cont];

	switch (pi->mode) {

	case 0:
	case 1:
	case 2:	w0(0x60+r); w2(1); w0(val); w2(4);
		break;

	case 3:
	case 4:
	case 5: w3(0x40+r); w4(val);
		break;

	}
}

static int epat_read_regr( PIA *pi, int cont, int regr )

{	int  a, b, r;

	r = regr + cont_map[cont];

	switch (pi->mode) {

	case 0:	w0(r); w2(1); w2(3); 
		a = r1(); w2(4); b = r1();
		return j44(a,b);

	case 1: w0(0x40+r); w2(1); w2(4);
		a = r1(); b = r2(); w0(0xff);
		return j53(a,b);

	case 2: w0(0x20+r); w2(1); w2(0x25);
		a = r0(); w2(4);
		return a;

	case 3:
	case 4:
	case 5: w3(r); w2(0x24); a = r4(); w2(4);
		return a;

	}
	return -1;	/* never gets here */
}

static void epat_read_block( PIA *pi, char * buf, int count )

{	int  k, ph, a, b;

	switch (pi->mode) {

	case 0:	w0(7); w2(1); w2(3); w0(0xff);
		ph = 0;
		for(k=0;k<count;k++) {
			if (k == count-1) w0(0xfd);
			w2(6+ph); a = r1();
			if (a & 8) b = a; 
			  else { w2(4+ph); b = r1(); }
			buf[k] = j44(a,b);
			ph =  1 - ph;
		}
		w0(0); w2(4);
		break;

	case 1: w0(0x47); w2(1); w2(5); w0(0xff);
		ph = 0;
		for(k=0;k<count;k++) {
			if (k == count-1) w0(0xfd); 
			w2(4+ph);
			a = r1(); b = r2();
			buf[k] = j53(a,b);
			ph = 1 - ph;
		}
		w0(0); w2(4);
		break;

	case 2: w0(0x27); w2(1); w2(0x25); w0(0);
		ph = 0;
		for(k=0;k<count-1;k++) {
			w2(0x24+ph);
			buf[k] = r0();
			ph = 1 - ph;
		}
		w2(0x26); w2(0x27); buf[count-1] = r0(); 
		w2(0x25); w2(4);
		break;

	case 3: w3(0x80); w2(0x24);
		for(k=0;k<count-1;k++) buf[k] = r4();
		w2(4); w3(0xa0); w2(0x24); buf[count-1] = r4();
		w2(4);
		break;

	case 4: w3(0x80); w2(0x24);
		for(k=0;k<(count/2)-1;k++) ((u16 *)buf)[k] = r4w();
		buf[count-2] = r4();
		w2(4); w3(0xa0); w2(0x24); buf[count-1] = r4();
		w2(4);
		break;

	case 5: w3(0x80); w2(0x24);
		for(k=0;k<(count/4)-1;k++) ((u32 *)buf)[k] = r4l();
		for(k=count-4;k<count-1;k++) buf[k] = r4();
		w2(4); w3(0xa0); w2(0x24); buf[count-1] = r4();
		w2(4);
		break;

	}
}

static void epat_write_block( PIA *pi, char * buf, int count )   

{	int ph, k;

	switch (pi->mode) {

	case 0:
	case 1:
	case 2: w0(0x67); w2(1); w2(5);
		ph = 0;
		for(k=0;k<count;k++) {
		  	w0(buf[k]);
			w2(4+ph);
			ph = 1 - ph;
		}
		w2(7); w2(4);
		break;

	case 3: w3(0xc0); 
		for(k=0;k<count;k++) w4(buf[k]);
		w2(4);
		break;

	case 4: w3(0xc0); 
		for(k=0;k<(count/2);k++) w4w(((u16 *)buf)[k]);
		w2(4);
		break;

	case 5: w3(0xc0); 
		for(k=0;k<(count/4);k++) w4l(((u32 *)buf)[k]);
		w2(4);
		break;

	}
}

/* these macros access the EPAT registers in native addressing */

#define	WR(r,v)		epat_write_regr(pi,2,r,v)
#define	RR(r)		(epat_read_regr(pi,2,r))

/* and these access the IDE task file */

#define WRi(r,v)         epat_write_regr(pi,0,r,v)
#define RRi(r)           (epat_read_regr(pi,0,r))

/* FIXME:  the CPP stuff should be fixed to handle multiple EPATs on a chain */

#define CPP(x) 	w2(4);w0(0x22);w0(0xaa);w0(0x55);w0(0);w0(0xff);\
                w0(0x87);w0(0x78);w0(x);w2(4);w2(5);w2(4);w0(0xff);

static void epat_connect ( PIA *pi )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();

 	/* Initialize the chip */
	CPP(0);

	if (epatc8) {
		CPP(0x40);CPP(0xe0);
		w0(0);w2(1);w2(4);
		WR(0x8,0x12);WR(0xc,0x14);WR(0x12,0x10);
		WR(0xe,0xf);WR(0xf,4);
		/* WR(0xe,0xa);WR(0xf,4); */
		WR(0xe,0xd);WR(0xf,0);
		/* CPP(0x30); */
	}

        /* Connect to the chip */
	CPP(0xe0);
        w0(0);w2(1);w2(4); /* Idle into SPP */
        if (pi->mode >= 3) {
          w0(0);w2(1);w2(4);w2(0xc);
          /* Request EPP */
          w0(0x40);w2(6);w2(7);w2(4);w2(0xc);w2(4);
        }

	if (!epatc8) {
		WR(8,0x10); WR(0xc,0x14); WR(0xa,0x38); WR(0x12,0x10);
	}
}

static void epat_disconnect (PIA *pi)
{	CPP(0x30);
	w0(pi->saved_r0);
	w2(pi->saved_r2);
}

static int epat_test_proto( PIA *pi, char * scratch, int verbose )

{       int     k, j, f, cc;
	int	e[2] = {0,0};

        epat_connect(pi);
	cc = RR(0xd);
	epat_disconnect(pi);

	epat_connect(pi);
	for (j=0;j<2;j++) {
  	    WRi(6,0xa0+j*0x10);
            for (k=0;k<256;k++) {
                WRi(2,k^0xaa);
                WRi(3,k^0x55);
                if (RRi(2) != (k^0xaa)) e[j]++;
                }
	    }
        epat_disconnect(pi);

        f = 0;
        epat_connect(pi);
        WR(0x13,1); WR(0x13,0); WR(0xa,0x11);
        epat_read_block(pi,scratch,512);
	
        for (k=0;k<256;k++) {
            if ((scratch[2*k] & 0xff) != k) f++;
            if ((scratch[2*k+1] & 0xff) != (0xff-k)) f++;
        }
        epat_disconnect(pi);

        if (verbose)  {
            printk("%s: epat: port 0x%x, mode %d, ccr %x, test=(%d,%d,%d)\n",
		   pi->device,pi->port,pi->mode,cc,e[0],e[1],f);
	}
	
        return (e[0] && e[1]) || f;
}

static void epat_log_adapter( PIA *pi, char * scratch, int verbose )

{	int	ver;
        char    *mode_string[6] = 
		   {"4-bit","5/3","8-bit","EPP-8","EPP-16","EPP-32"};

	epat_connect(pi);
	WR(0xa,0x38);		/* read the version code */
        ver = RR(0xb);
        epat_disconnect(pi);

	printk("%s: epat %s, Shuttle EPAT chip %x at 0x%x, ",
		pi->device,EPAT_VERSION,ver,pi->port);
	printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol epat = {
	.owner		= THIS_MODULE,
	.name		= "epat",
	.max_mode	= 6,
	.epp_first	= 3,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= epat_write_regr,
	.read_regr	= epat_read_regr,
	.write_block	= epat_write_block,
	.read_block	= epat_read_block,
	.connect	= epat_connect,
	.disconnect	= epat_disconnect,
	.test_proto	= epat_test_proto,
	.log_adapter	= epat_log_adapter,
};

static int __init epat_init(void)
{
#ifdef CONFIG_PARIDE_EPATC8
	epatc8 = 1;
#endif
	return paride_register(&epat);
}

static void __exit epat_exit(void)
{
	paride_unregister(&epat);
}

MODULE_LICENSE("GPL");
module_init(epat_init)
module_exit(epat_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /* 
        epia.c    (c) 1997-8  Grant R. Guenther <grant@torque.net>
                              Under the terms of the GNU General Public License.

        epia.c is a low-level protocol driver for Shuttle Technologies 
	EPIA parallel to IDE adapter chip.  This device is now obsolete
	and has been replaced with the EPAT chip, which is supported
	by epat.c, however, some devices based on EPIA are still
	available.

*/

/* Changes:

        1.01    GRG 1998.05.06 init_proto, release_proto
	1.02    GRG 1998.06.17 support older versions of EPIA

*/

#define EPIA_VERSION      "1.02"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

/* mode codes:  0  nybble reads on port 1, 8-bit writes
                1  5/3 reads on ports 1 & 2, 8-bit writes
                2  8-bit reads and writes
                3  8-bit EPP mode
		4  16-bit EPP
		5  32-bit EPP
*/

#define j44(a,b)                (((a>>4)&0x0f)+(b&0xf0))
#define j53(a,b)                (((a>>3)&0x1f)+((b<<4)&0xe0))

/* cont =  0   IDE register file
   cont =  1   IDE control registers
*/

static int cont_map[2] = { 0, 0x80 };

static int epia_read_regr( PIA *pi, int cont, int regr )

{       int     a, b, r;

	regr += cont_map[cont];

        switch (pi->mode)  {

        case 0: r = regr^0x39;
                w0(r); w2(1); w2(3); w0(r);
                a = r1(); w2(1); b = r1(); w2(4);
                return j44(a,b);

        case 1: r = regr^0x31;
                w0(r); w2(1); w0(r&0x37); 
                w2(3); w2(5); w0(r|0xf0);
                a = r1(); b = r2(); w2(4);
                return j53(a,b);

        case 2: r = regr^0x29;
                w0(r); w2(1); w2(0X21); w2(0x23); 
                a = r0(); w2(4);
                return a;

	case 3:
	case 4:
        case 5: w3(regr); w2(0x24); a = r4(); w2(4);
                return a;

        }
        return -1;
}       

static void epia_write_regr( PIA *pi, int cont, int regr, int val)

{       int  r;

	regr += cont_map[cont];

        switch (pi->mode)  {

        case 0:
        case 1:
        case 2: r = regr^0x19;
                w0(r); w2(1); w0(val); w2(3); w2(4);
                break;

	case 3:
	case 4:
        case 5: r = regr^0x40;
                w3(r); w4(val); w2(4);
                break;
        }
}

#define WR(r,v)         epia_write_regr(pi,0,r,v)
#define RR(r)           (epia_read_regr(pi,0,r))

/* The use of register 0x84 is entirely unclear - it seems to control
   some EPP counters ...  currently we know about 3 different block
   sizes:  the standard 512 byte reads and writes, 12 byte writes and 
   2048 byte reads (the last two being used in the CDrom drivers.
*/

static void epia_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();

        w2(4); w0(0xa0); w0(0x50); w0(0xc0); w0(0x30); w0(0xa0); w0(0);
        w2(1); w2(4);
        if (pi->mode >= 3) { 
                w0(0xa); w2(1); w2(4); w0(0x82); w2(4); w2(0xc); w2(4);
                w2(0x24); w2(0x26); w2(4);
        }
        WR(0x86,8);  
}

static void epia_disconnect ( PIA *pi )

{       /* WR(0x84,0x10); */
        w0(pi->saved_r0);
        w2(1); w2(4);
        w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void epia_read_block( PIA *pi, char * buf, int count )

{       int     k, ph, a, b;

        switch (pi->mode) {

        case 0: w0(0x81); w2(1); w2(3); w0(0xc1);
                ph = 1;
                for (k=0;k<count;k++) {
                        w2(2+ph); a = r1();
                        w2(4+ph); b = r1();
                        buf[k] = j44(a,b);
                        ph = 1 - ph;
                } 
                w0(0); w2(4);
                break;

        case 1: w0(0x91); w2(1); w0(0x10); w2(3); 
                w0(0x51); w2(5); w0(0xd1); 
                ph = 1;
                for (k=0;k<count;k++) {
                        w2(4+ph);
                        a = r1(); b = r2();
                        buf[k] = j53(a,b);
                        ph = 1 - ph;
                }
                w0(0); w2(4);
                break;

        case 2: w0(0x89); w2(1); w2(0x23); w2(0x21); 
                ph = 1;
                for (k=0;k<count;k++) {
                        w2(0x24+ph);
                        buf[k] = r0();
                        ph = 1 - ph;
                }
                w2(6); w2(4);
                break;

        case 3: if (count > 512) WR(0x84,3);
		w3(0); w2(0x24);
                for (k=0;k<count;k++) buf[k] = r4();
                w2(4); WR(0x84,0);
                break;

        case 4: if (count > 512) WR(0x84,3);
		w3(0); w2(0x24);
		for (k=0;k<count/2;k++) ((u16 *)buf)[k] = r4w();
                w2(4); WR(0x84,0);
                break;

        case 5: if (count > 512) WR(0x84,3);
		w3(0); w2(0x24);
                for (k=0;k<count/4;k++) ((u32 *)buf)[k] = r4l();
                w2(4); WR(0x84,0);
                break;

        }
}

static void epia_write_block( PIA *pi, char * buf, int count )

{       int     ph, k, last, d;

        switch (pi->mode) {

        case 0:
        case 1:
        case 2: w0(0xa1); w2(1); w2(3); w2(1); w2(5);
                ph = 0;  last = 0x8000;
                for (k=0;k<count;k++) {
                        d = buf[k];
                        if (d != last) { last = d; w0(d); }
                        w2(4+ph);
                        ph = 1 - ph;
                }
                w2(7); w2(4);
                break;

        case 3: if (count < 512) WR(0x84,1);
		w3(0x40);
                for (k=0;k<count;k++) w4(buf[k]);
		if (count < 512) WR(0x84,0);
                break;

        case 4: if (count < 512) WR(0x84,1);
		w3(0x40);
                for (k=0;k<count/2;k++) w4w(((u16 *)buf)[k]);
		if (count < 512) WR(0x84,0);
                break;

        case 5: if (count < 512) WR(0x84,1);
		w3(0x40);
                for (k=0;k<count/4;k++) w4l(((u32 *)buf)[k]);
		if (count < 512) WR(0x84,0);
                break;

        }

}

static int epia_test_proto( PIA *pi, char * scratch, int verbose )

{       int     j, k, f;
	int	e[2] = {0,0};

        epia_connect(pi);
        for (j=0;j<2;j++) {
            WR(6,0xa0+j*0x10);
            for (k=0;k<256;k++) {
                WR(2,k^0xaa);
                WR(3,k^0x55);
                if (RR(2) != (k^0xaa)) e[j]++;
                }
	    WR(2,1); WR(3,1);
            }
        epia_disconnect(pi);

        f = 0;
        epia_connect(pi);
        WR(0x84,8);
        epia_read_block(pi,scratch,512);
        for (k=0;k<256;k++) {
            if ((scratch[2*k] & 0xff) != ((k+1) & 0xff)) f++;
            if ((scratch[2*k+1] & 0xff) != ((-2-k) & 0xff)) f++;
        }
        WR(0x84,0);
        epia_disconnect(pi);

        if (verbose)  {
            printk("%s: epia: port 0x%x, mode %d, test=(%d,%d,%d)\n",
                   pi->device,pi->port,pi->mode,e[0],e[1],f);
        }
        
        return (e[0] && e[1]) || f;

}


static void epia_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[6] = {"4-bit","5/3","8-bit",
				   "EPP-8","EPP-16","EPP-32"};

        printk("%s: epia %s, Shuttle EPIA at 0x%x, ",
                pi->device,EPIA_VERSION,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

}

static struct pi_protocol epia = {
	.owner		= THIS_MODULE,
	.name		= "epia",
	.max_mode	= 6,
	.epp_first	= 3,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= epia_write_regr,
	.read_regr	= epia_read_regr,
	.write_block	= epia_write_block,
	.read_block	= epia_read_block,
	.connect	= epia_connect,
	.disconnect	= epia_disconnect,
	.test_proto	= epia_test_proto,
	.log_adapter	= epia_log_adapter,
};

static int __init epia_init(void)
{
	return paride_register(&epia);
}

static void __exit epia_exit(void)
{
	paride_unregister(&epia);
}

MODULE_LICENSE("GPL");
module_init(epia_init)
module_exit(epia_exit)
                                                                                                                                               /* 
        fit2.c        (c) 1998  Grant R. Guenther <grant@torque.net>
                          Under the terms of the GNU General Public License.

	fit2.c is a low-level protocol driver for the older version
        of the Fidelity International Technology parallel port adapter.  
	This adapter is used in their TransDisk 2000 and older TransDisk
	3000 portable hard-drives.  As far as I can tell, this device
	supports 4-bit mode _only_.  

	Newer models of the FIT products use an enhanced protocol.
	The "fit3" protocol module should support current drives.

*/

#define FIT2_VERSION      "1.0"

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

NB:  The FIT adapter does not appear to use the control registers.
So, we map ALT_STATUS to STATUS and NO-OP writes to the device
control register - this means that IDE reset will not work on these
devices.

*/

static void  fit2_write_regr( PIA *pi, int cont, int regr, int val)

{	if (cont == 1) return;
	w2(0xc); w0(regr); w2(4); w0(val); w2(5); w0(0); w2(4);
}

static int fit2_read_regr( PIA *pi, int cont, int regr )

{	int  a, b, r;

	if (cont) {
	  if (regr != 6) return 0xff;
	  r = 7;
	} else r = regr + 0x10;

	w2(0xc); w0(r); w2(4); w2(5); 
	         w0(0); a = r1();
	         w0(1); b = r1();
	w2(4);

	return j44(a,b);

}

static void fit2_read_block( PIA *pi, char * buf, int count )

{	int  k, a, b, c, d;

	w2(0xc); w0(0x10);

	for (k=0;k<count/4;k++) {

		w2(4); w2(5);
		w0(0); a = r1(); w0(1); b = r1();
		w0(3); c = r1(); w0(2); d = r1(); 
		buf[4*k+0] = j44(a,b);
		buf[4*k+1] = j44(d,c);

                w2(4); w2(5);
                       a = r1(); w0(3); b = r1();
                w0(1); c = r1(); w0(0); d = r1(); 
                buf[4*k+2] = j44(d,c);
                buf[4*k+3] = j44(a,b);

	}

	w2(4);

}

static void fit2_write_block( PIA *pi, char * buf, int count )

{	int k;


	w2(0xc); w0(0); 
	for (k=0;k<count/2;k++) {
		w2(4); w0(buf[2*k]); 
		w2(5); w0(buf[2*k+1]);
	}
	w2(4);
}

static void fit2_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	w2(0xcc); 
}

static void fit2_disconnect ( PIA *pi )

{       w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void fit2_log_adapter( PIA *pi, char * scratch, int verbose )

{       printk("%s: fit2 %s, FIT 2000 adapter at 0x%x, delay %d\n",
                pi->device,FIT2_VERSION,pi->port,pi->delay);

}

static struct pi_protocol fit2 = {
	.owner		= THIS_MODULE,
	.name		= "fit2",
	.max_mode	= 1,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= fit2_write_regr,
	.read_regr	= fit2_read_regr,
	.write_block	= fit2_write_block,
	.read_block	= fit2_read_block,
	.connect	= fit2_connect,
	.disconnect	= fit2_disconnect,
	.log_adapter	= fit2_log_adapter,
};

static int __init fit2_init(void)
{
	return paride_register(&fit2);
}

static void __exit fit2_exit(void)
{
	paride_unregister(&fit2);
}

MODULE_LICENSE("GPL");
module_init(fit2_init)
module_exit(fit2_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /* 
        fit3.c        (c) 1998  Grant R. Guenther <grant@torque.net>
                          Under the terms of the GNU General Public License.

	fit3.c is a low-level protocol driver for newer models 
        of the Fidelity International Technology parallel port adapter.  
	This adapter is used in their TransDisk 3000 portable 
	hard-drives, as well as CD-ROM, PD-CD and other devices.

	The TD-2000 and certain older devices use a different protocol.
	Try the fit2 protocol module with them.

        NB:  The FIT adapters do not appear to support the control 
	registers.  So, we map ALT_STATUS to STATUS and NO-OP writes 
	to the device control register - this means that IDE reset 
	will not work on these devices.

*/

#define FIT3_VERSION      "1.0"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#define j44(a,b)                (((a>>3)&0x0f)|((b<<1)&0xf0))

#define w7(byte)                {out_p(7,byte);}
#define r7()                    (in_p(7) & 0xff)

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 

*/

static void  fit3_write_regr( PIA *pi, int cont, int regr, int val)

{	if (cont == 1) return;

	switch (pi->mode) {

	case 0:
	case 1: w2(0xc); w0(regr); w2(0x8); w2(0xc); 
		w0(val); w2(0xd); 
		w0(0);   w2(0xc);
		break;

	case 2: w2(0xc); w0(regr); w2(0x8); w2(0xc);
		w4(val); w4(0);
		w2(0xc);
		break;

	}
}

static int fit3_read_regr( PIA *pi, int cont, int regr )

{	int  a, b;

	if (cont) {
	  if (regr != 6) return 0xff;
	  regr = 7;
	} 

	switch (pi->mode) {

	case 0: w2(0xc); w0(regr + 0x10); w2(0x8); w2(0xc);
		w2(0xd); a = r1();
		w2(0xf); b = r1(); 
		w2(0xc);
		return j44(a,b);

	case 1: w2(0xc); w0(regr + 0x90); w2(0x8); w2(0xc);
		w2(0xec); w2(0xee); w2(0xef); a = r0(); 
		w2(0xc);
		return a;

	case 2: w2(0xc); w0(regr + 0x90); w2(0x8); w2(0xc); 
		w2(0xec); 
		a = r4(); b = r4(); 
		w2(0xc);
		return a;

	}
	return -1; 

}

static void fit3_read_block( PIA *pi, char * buf, int count )

{	int  k, a, b, c, d;

	switch (pi->mode) {

	case 0: w2(0xc); w0(0x10); w2(0x8); w2(0xc);
		for (k=0;k<count/2;k++) {
		    w2(0xd); a = r1();
		    w2(0xf); b = r1();
		    w2(0xc); c = r1();
		    w2(0xe); d = r1();
		    buf[2*k  ] = j44(a,b);
		    buf[2*k+1] = j44(c,d);
		}
		w2(0xc);
		break;

	case 1: w2(0xc); w0(0x90); w2(0x8); w2(0xc); 
		w2(0xec); w2(0xee);
		for (k=0;k<count/2;k++) {
		    w2(0xef); a = r0();
		    w2(0xee); b = r0();
                    buf[2*k  ] = a;
                    buf[2*k+1] = b;
		}
		w2(0xec); 
		w2(0xc);
		break;

	case 2: w2(0xc); w0(0x90); w2(0x8); w2(0xc); 
                w2(0xec);
		for (k=0;k<count;k++) buf[k] = r4();
                w2(0xc);
		break;

	}
}

static void fit3_write_block( PIA *pi, char * buf, int count )

{	int k;

        switch (pi->mode) {

	case 0:
        case 1: w2(0xc); w0(0); w2(0x8); w2(0xc);
                for (k=0;k<count/2;k++) {
 		    w0(buf[2*k  ]); w2(0xd);
 		    w0(buf[2*k+1]); w2(0xc);
		}
		break;

        case 2: w2(0xc); w0(0); w2(0x8); w2(0xc); 
                for (k=0;k<count;k++) w4(buf[k]);
                w2(0xc);
		break;
	}
}

static void fit3_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	w2(0xc); w0(0); w2(0xa);
	if (pi->mode == 2) { 
		w2(0xc); w0(0x9); w2(0x8); w2(0xc); 
		}
}

static void fit3_disconnect ( PIA *pi )

{       w2(0xc); w0(0xa); w2(0x8); w2(0xc);
	w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static void fit3_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[3] = {"4-bit","8-bit","EPP"};

	printk("%s: fit3 %s, FIT 3000 adapter at 0x%x, "
	       "mode %d (%s), delay %d\n",
                pi->device,FIT3_VERSION,pi->port,
		pi->mode,mode_string[pi->mode],pi->delay);

}

static struct pi_protocol fit3 = {
	.owner		= THIS_MODULE,
	.name		= "fit3",
	.max_mode	= 3,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= fit3_write_regr,
	.read_regr	= fit3_read_regr,
	.write_block	= fit3_write_block,
	.read_block	= fit3_read_block,
	.connect	= fit3_connect,
	.disconnect	= fit3_disconnect,
	.log_adapter	= fit3_log_adapter,
};

static int __init fit3_init(void)
{
	return paride_register(&fit3);
}

static void __exit fit3_exit(void)
{
	paride_unregister(&fit3);
}

MODULE_LICENSE("GPL");
module_init(fit3_init)
module_exit(fit3_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /* 
	friq.c	(c) 1998    Grant R. Guenther <grant@torque.net>
		            Under the terms of the GNU General Public License

	friq.c is a low-level protocol driver for the Freecom "IQ"
	parallel port IDE adapter.   Early versions of this adapter
	use the 'frpw' protocol.
	
	Freecom uses this adapter in a battery powered external 
	CD-ROM drive.  It is also used in LS-120 drives by
	Maxell and Panasonic, and other devices.

	The battery powered drive requires software support to
	control the power to the drive.  This module enables the
	drive power when the high level driver (pcd) is loaded
	and disables it when the module is unloaded.  Note, if
	the friq module is built in to the kernel, the power
	will never be switched off, so other means should be
	used to conserve battery power.

*/

/* Changes:

	1.01	GRG 1998.12.20	 Added support for soft power switch
*/

#define	FRIQ_VERSION	"1.01" 

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#define CMD(x)		w2(4);w0(0xff);w0(0xff);w0(0x73);w0(0x73);\
			w0(0xc9);w0(0xc9);w0(0x26);w0(0x26);w0(x);w0(x);

#define j44(l,h)	(((l>>4)&0x0f)|(h&0xf0))

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int  cont_map[2] = { 0x08, 0x10 };

static int friq_read_regr( PIA *pi, int cont, int regr )

{	int	h,l,r;

	r = regr + cont_map[cont];

	CMD(r);
	w2(6); l = r1();
	w2(4); h = r1();
	w2(4); 

	return j44(l,h);

}

static void friq_write_regr( PIA *pi, int cont, int regr, int val)

{	int r;

        r = regr + cont_map[cont];

	CMD(r);
	w0(val);
	w2(5);w2(7);w2(5);w2(4);
}

static void friq_read_block_int( PIA *pi, char * buf, int count, int regr )

{       int     h, l, k, ph;

        switch(pi->mode) {

        case 0: CMD(regr); 
                for (k=0;k<count;k++) {
                        w2(6); l = r1();
                        w2(4); h = r1();
                        buf[k] = j44(l,h);
                }
                w2(4);
                break;

        case 1: ph = 2;
                CMD(regr+0xc0); 
                w0(0xff);
                for (k=0;k<count;k++) {
                        w2(0xa4 + ph); 
                        buf[k] = r0();
                        ph = 2 - ph;
                } 
                w2(0xac); w2(0xa4); w2(4);
                break;

	case 2: CMD(regr+0x80);
		for (k=0;k<count-2;k++) buf[k] = r4();
		w2(0xac); w2(0xa4);
		buf[count-2] = r4();
		buf[count-1] = r4();
		w2(4);
		break;

	case 3: CMD(regr+0x80);
                for (k=0;k<(count/2)-1;k++) ((u16 *)buf)[k] = r4w();
                w2(0xac); w2(0xa4);
                buf[count-2] = r4();
                buf[count-1] = r4();
                w2(4);
                break;

	case 4: CMD(regr+0x80);
                for (k=0;k<(count/4)-1;k++) ((u32 *)buf)[k] = r4l();
                buf[count-4] = r4();
                buf[count-3] = r4();
                w2(0xac); w2(0xa4);
                buf[count-2] = r4();
                buf[count-1] = r4();
                w2(4);
                break;

        }
}

static void friq_read_block( PIA *pi, char * buf, int count)

{	friq_read_block_int(pi,buf,count,0x08);
}

static void friq_write_block( PIA *pi, char * buf, int count )
 
{	int	k;

	switch(pi->mode) {

	case 0:
	case 1: CMD(8); w2(5);
        	for (k=0;k<count;k++) {
			w0(buf[k]);
			w2(7);w2(5);
		}
		w2(4);
		break;

	case 2: CMD(0xc8); w2(5);
		for (k=0;k<count;k++) w4(buf[k]);
		w2(4);
		break;

        case 3: CMD(0xc8); w2(5);
                for (k=0;k<count/2;k++) w4w(((u16 *)buf)[k]);
                w2(4);
                break;

        case 4: CMD(0xc8); w2(5);
                for (k=0;k<count/4;k++) w4l(((u32 *)buf)[k]);
                w2(4);
                break;
	}
}

static void friq_connect ( PIA *pi  )

{       pi->saved_r0 = r0();
        pi->saved_r2 = r2();
	w2(4);
}

static void friq_disconnect ( PIA *pi )

{       CMD(0x20);
	w0(pi->saved_r0);
        w2(pi->saved_r2);
} 

static int friq_test_proto( PIA *pi, char * scratch, int verbose )

{       int     j, k, r;
	int	e[2] = {0,0};

	pi->saved_r0 = r0();	
	w0(0xff); udelay(20); CMD(0x3d); /* turn the power on */
	udelay(500);
	w0(pi->saved_r0);

	friq_connect(pi);
	for (j=0;j<2;j++) {
                friq_write_regr(pi,0,6,0xa0+j*0x10);
                for (k=0;k<256;k++) {
                        friq_write_regr(pi,0,2,k^0xaa);
                        friq_write_regr(pi,0,3,k^0x55);
                        if (friq_read_regr(pi,0,2) != (k^0xaa)) e[j]++;
                        }
                }
	friq_disconnect(pi);

	friq_connect(pi);
        friq_read_block_int(pi,scratch,512,0x10);
        r = 0;
        for (k=0;k<128;k++) if (scratch[k] != k) r++;
	friq_disconnect(pi);

        if (verbose)  {
            printk("%s: friq: port 0x%x, mode %d, test=(%d,%d,%d)\n",
                   pi->device,pi->port,pi->mode,e[0],e[1],r);
        }

        return (r || (e[0] && e[1]));
}


static void friq_log_adapter( PIA *pi, char * scratch, int verbose )

{       char    *mode_string[6] = {"4-bit","8-bit",
				   "EPP-8","EPP-16","EPP-32"};

        printk("%s: friq %s, Freecom IQ ASIC-2 adapter at 0x%x, ", pi->device,
		FRIQ_VERSION,pi->port);
        printk("mode %d (%s), delay %d\n",pi->mode,
		mode_string[pi->mode],pi->delay);

	pi->private = 1;
	friq_connect(pi);
	CMD(0x9e);  		/* disable sleep timer */
	friq_disconnect(pi);

}

static void friq_release_proto( PIA *pi)
{
	if (pi->private) {		/* turn off the power */
		friq_connect(pi);
		CMD(0x1d); CMD(0x1e);
		friq_disconnect(pi);
		pi->private = 0;
	}
}

static struct pi_protocol friq = {
	.owner		= THIS_MODULE,
	.name		= "friq",
	.max_mode	= 5,
	.epp_first	= 2,
	.default_delay	= 1,
	.max_units	= 1,
	.write_regr	= friq_write_regr,
	.read_regr	= friq_read_regr,
	.write_block	= friq_write_block,
	.read_block	= friq_read_block,
	.connect	= friq_connect,
	.disconnect	= friq_disconnect,
	.test_proto	= friq_test_proto,
	.log_adapter	= friq_log_adapter,
	.release_proto	= friq_release_proto,
};

static int __init friq_init(void)
{
	return paride_register(&friq);
}

static void __exit friq_exit(void)
{
	paride_unregister(&friq);
}

MODULE_LICENSE("GPL");
module_init(friq_init)
module_exit(friq_exit)
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /* 
	frpw.c	(c) 1996-8  Grant R. Guenther <grant@torque.net>
		            Under the terms of the GNU General Public License

	frpw.c is a low-level protocol driver for the Freecom "Power"
	parallel port IDE adapter.
	
	Some applications of this adapter may require a "printer" reset
	prior to loading the driver.  This can be done by loading and
	unloading the "lp" driver, or it can be done by this driver
	if you define FRPW_HARD_RESET.  The latter is not recommended
	as it may upset devices on other ports.

*/

/* Changes:

        1.01    GRG 1998.05.06 init_proto, release_proto
			       fix chip detect
			       added EPP-16 and EPP-32
	1.02    GRG 1998.09.23 added hard reset to initialisation process
	1.03    GRG 1998.12.14 made hard reset conditional

*/

#define	FRPW_VERSION	"1.03" 

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/io.h>

#include "paride.h"

#define cec4		w2(0xc);w2(0xe);w2(0xe);w2(0xc);w2(4);w2(4);w2(4);
#define j44(l,h)	(((l>>4)&0x0f)|(h&0xf0))

/* cont = 0 - access the IDE register file 
   cont = 1 - access the IDE command set 
*/

static int  cont_map[2] = { 0x08, 0x10 };

static int frpw_read_regr( PIA *pi, int cont, int regr )

{	int	h,l,r;

	r = regr + cont_map[cont];

	w2(4);
	w0(r); cec4;
	w2(6); l = r1();
	w2(4); h = r1();
	w2(4); 

	return j44(l,h);

}

static void frpw_write_regr( PIA *pi, int cont, int regr, int val)

{	int r;

        r = regr + cont_map[cont];

	w2(4); w0(r); cec4; 
	w0(val);
	w2(5);w2(7);w2(5);w2(4);
}

static void frpw_read_block_int( PIA *pi, char * buf, int count, int regr )

{       int     h, l, k, ph;

        switch(pi->mod