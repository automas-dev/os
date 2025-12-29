// #include <string>

// #include "test_common.h"

// extern "C" {
// #include "libk/sys_call.h"
// }

// extern "C" {
// FAKE_VALUE_FUNC(uint32_t, send_call, uint32_t, uint32_t, uint32_t, uint32_t);
// FAKE_VALUE_FUNC(uint32_t, send_call_noret, uint32_t, uint32_t, uint32_t, uint32_t);
// }

// class LibK : public ::testing::Test {
// protected:
//     void SetUp() override {
//         init_mocks();

//         RESET_FAKE(send_call);
//         RESET_FAKE(send_call_noret);
//     }
// };

// // TEST_F(LibK, malloc) {
// //     send_call_fake.return_val = 3;
// //     void * ptr                     = _malloc(12);
// //     EXPECT_EQ(send_call_fake.call_count, 1);
// //     EXPECT_EQ(send_call_fake.arg0_val, 0x200);
// //     EXPECT_EQ(send_call_fake.arg1_val, 12);
// //     EXPECT_EQ(ptr, (void *)3);
// // }

// // TEST_F(LibK, realloc) {
// //     send_call_fake.return_val = 3;
// //     void * ptr                     = _realloc((void *)5, 12);
// //     EXPECT_EQ(send_call_fake.call_count, 1);
// //     EXPECT_EQ(send_call_fake.arg0_val, 0x201);
// //     EXPECT_EQ(send_call_fake.arg1_val, 5);
// //     EXPECT_EQ(send_call_fake.arg2_val, 12);
// //     EXPECT_EQ(ptr, (void *)3);
// // }

// // TEST_F(LibK, free) {
// //     _free((void *)5);
// //     EXPECT_EQ(send_call_fake.call_count, 1);
// //     EXPECT_EQ(send_call_fake.arg0_val, 0x202);
// //     EXPECT_EQ(send_call_fake.arg1_val, 5);
// // }

// TEST_F(LibK, sys_io_open) {
//     send_call_fake.return_val = 3;
//     EXPECT_EQ(3, _sys_io_open("path", "mode"));
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x100);
// }

// TEST_F(LibK, sys_io_close) {
//     send_call_fake.return_val = 3;
//     EXPECT_EQ(3, _sys_io_close(2));
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x101);
//     EXPECT_EQ(send_call_fake.arg1_val, 2);
// }

// TEST_F(LibK, sys_io_read) {
//     send_call_fake.return_val = 3;
//     char buff[2]              = "a";
//     EXPECT_EQ(3, _sys_io_read(0, buff, 1));
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x102);
//     EXPECT_EQ(send_call_fake.arg1_val, 0);
//     EXPECT_EQ(send_call_fake.arg2_val, (uint32_t)buff);
//     EXPECT_EQ(send_call_fake.arg3_val, 1);
// }

// TEST_F(LibK, sys_io_write) {
//     send_call_fake.return_val = 3;
//     char buff[2]              = "a";
//     EXPECT_EQ(3, _sys_io_write(0, buff, 1));
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x103);
//     EXPECT_EQ(send_call_fake.arg1_val, 0);
//     EXPECT_EQ(send_call_fake.arg2_val, (uint32_t)buff);
//     EXPECT_EQ(send_call_fake.arg3_val, 1);
// }

// TEST_F(LibK, sys_io_seek) {
//     send_call_fake.return_val = 3;
//     char buff[2]              = "a";
//     EXPECT_EQ(3, _sys_io_seek(1, 2, 3));
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x104);
//     EXPECT_EQ(send_call_fake.arg1_val, 1);
//     EXPECT_EQ(send_call_fake.arg2_val, 2);
//     EXPECT_EQ(send_call_fake.arg3_val, 3);
// }

// TEST_F(LibK, sys_io_tell) {
//     send_call_fake.return_val = 3;
//     char buff[2]              = "a";
//     EXPECT_EQ(3, _sys_io_tell(0));
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x105);
//     EXPECT_EQ(send_call_fake.arg1_val, 0);
// }

// TEST_F(LibK, page_alloc) {
//     send_call_fake.return_val = 3;
//     EXPECT_EQ((void *)3, _sys_page_alloc(1));
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x203);
//     EXPECT_EQ(send_call_fake.arg1_val, 1);
// }

// TEST_F(LibK, exit) {
//     _sys_proc_exit(200);
//     EXPECT_EQ(send_call_noret_fake.call_count, 1);
//     EXPECT_EQ(send_call_noret_fake.arg0_val, 0x300);
//     EXPECT_EQ(send_call_noret_fake.arg1_val, 200);
// }

// TEST_F(LibK, abort) {
//     const char * msg = "message";
//     _sys_proc_abort(200, msg);
//     EXPECT_EQ(send_call_noret_fake.call_count, 1);
//     EXPECT_EQ(send_call_noret_fake.arg0_val, 0x301);
//     EXPECT_EQ(send_call_noret_fake.arg1_val, 200);
//     EXPECT_EQ((void *)send_call_noret_fake.arg2_val, msg);
// }

// TEST_F(LibK, panic) {
//     const char * msg  = "message";
//     const char * file = "file";
//     unsigned int line = 37;
//     _sys_proc_panic(msg, file, line);
//     EXPECT_EQ(send_call_noret_fake.call_count, 1);
//     EXPECT_EQ(send_call_noret_fake.arg0_val, 0x302);
//     EXPECT_EQ((void *)send_call_noret_fake.arg1_val, msg);
//     EXPECT_EQ((void *)send_call_noret_fake.arg2_val, file);
//     EXPECT_EQ(send_call_noret_fake.arg3_val, line);
// }

// TEST_F(LibK, register_signals) {
//     _sys_register_signals((void *)1);
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x303);
//     EXPECT_EQ(send_call_fake.arg1_val, 1);
// }

// TEST_F(LibK, getpid) {
//     send_call_fake.return_val = 3;
//     EXPECT_EQ(3, _sys_proc_getpid());
//     ASSERT_EQ(1, send_call_fake.call_count);
//     EXPECT_EQ(0x304, send_call_fake.arg0_val);
// }

// TEST_F(LibK, queue_event) {
//     ebus_event_t event;
//     event.event_id   = EBUS_EVENT_TIMER;
//     event.timer.time = 2;
//     _sys_queue_event(&event);
//     ASSERT_EQ(1, send_call_fake.call_count);
//     EXPECT_EQ(0x305, send_call_fake.arg0_val);
//     EXPECT_EQ(&event, (void *)send_call_fake.arg1_val);
// }

// TEST_F(LibK, yield) {
//     send_call_fake.return_val = 2;
//     EXPECT_EQ(2, _sys_yield(5, (ebus_event_t *)3));
//     ASSERT_EQ(1, send_call_fake.call_count);
//     EXPECT_EQ(0x306, send_call_fake.arg0_val);
//     EXPECT_EQ(5, send_call_fake.arg1_val);
//     EXPECT_EQ(3, send_call_fake.arg2_val);
// }

// TEST_F(LibK, putc) {
//     send_call_fake.return_val = 1;
//     size_t olen               = _sys_putc('A');
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x1000);
//     EXPECT_EQ(send_call_fake.arg1_val, 'A');
//     EXPECT_EQ(olen, 1);
// }

// TEST_F(LibK, puts) {
//     const char * str          = "ABC";
//     send_call_fake.return_val = 3;
//     size_t olen               = _sys_puts(str);
//     EXPECT_EQ(send_call_fake.call_count, 1);
//     EXPECT_EQ(send_call_fake.arg0_val, 0x1001);
//     EXPECT_EQ(send_call_fake.arg1_val, (uint32_t)str);
//     EXPECT_EQ(olen, 3);
// }
