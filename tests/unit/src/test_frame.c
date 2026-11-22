#include <zephyr/ztest.h>
#include <string.h>
#include "frame.h"

ZTEST_SUITE(frame, NULL, NULL, NULL, NULL, NULL);

static struct sample ref(void)
{
	return (struct sample){.seq = 42, .uptime_ms = 12345, .temp_mc = -5125,
			       .accel_mg = {1, -2, 1000}, .gyro_mdps = {-3, 0, 114},
			       .mag_nt = {30000, -1000, -35000}, .alarms = 5};
}

ZTEST(frame, test_roundtrip)
{
	struct sample in = ref(), out = {0};
	char buf[FRAME_MAX_LEN];

	zassert_true(frame_encode(&in, buf, sizeof(buf)) > 0);
	zassert_equal(buf[0], '$');
	zassert_ok(frame_decode(buf, &out));
	zassert_mem_equal(&in, &out, sizeof(in));
}

ZTEST(frame, test_zero_sample_prefix)
{
	struct sample s = {0};
	char buf[FRAME_MAX_LEN];

	zassert_true(frame_encode(&s, buf, sizeof(buf)) > 0);
	zassert_equal(strncmp(buf, "$SN,0,0,0,", 10), 0);
	zassert_not_null(strchr(buf, '*'));
}

ZTEST(frame, test_corruption_detected)
{
	struct sample in = ref(), out;
	char buf[FRAME_MAX_LEN];

	frame_encode(&in, buf, sizeof(buf));
	buf[8] ^= 0x01; /* flip one bit in the body */
	zassert_equal(frame_decode(buf, &out), -EBADMSG);
}

ZTEST(frame, test_truncated_and_garbage)
{
	struct sample out;

	zassert_equal(frame_decode("", &out), -EBADMSG);
	zassert_equal(frame_decode("$SN,1,2", &out), -EBADMSG);
	zassert_equal(frame_decode("hello*0000", &out), -EBADMSG);
}

ZTEST(frame, test_buffer_too_small)
{
	struct sample in = ref();
	char tiny[16];

	zassert_equal(frame_encode(&in, tiny, sizeof(tiny)), -ENOMEM);
}
