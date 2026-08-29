#include <array>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <vector>

#include "test_common.h"

extern "C" {
#include "drivers/vga.h"
}

class VGA : public testing::Test {
protected:
    std::array<char, 80 * 25 * 2> buff;

    void SetUp() override {
        init_mocks();

        buff.fill(0);
        vga_init(buff.data());
    }
};

TEST_F(VGA, vga_clear) {
    buff.fill(0x82);
    vga_clear();
    for (size_t i = 0; i < buff.size(); i++) {
        if (i % 2 == 0) {
            EXPECT_EQ(' ', buff[i]);
        }
        else {
            EXPECT_EQ(7, buff[i]);
        }
    }
}

TEST_F(VGA, vga_put) {
    vga_put(3, 'Q', 0x21);

    EXPECT_EQ('Q', buff[6]);
    EXPECT_EQ(0x21, buff[7]);
}

TEST_F(VGA, vga_cursor_row) {
    EXPECT_EQ(0, vga_cursor_row());

    vga_cursor(10, 17);
    EXPECT_EQ(10, vga_cursor_row());
}

TEST_F(VGA, vga_cursor_col) {
    EXPECT_EQ(0, vga_cursor_col());

    vga_cursor(10, 17);
    EXPECT_EQ(17, vga_cursor_col());
}

TEST_F(VGA, vga_index) {
    EXPECT_EQ(0, vga_index());

    vga_cursor(10, 17);
    EXPECT_EQ(10 * VGA_COLS + 17, vga_index());
}

TEST_F(VGA, vga_cursor) {
    // invalid args
    vga_cursor(-1, 0);
    vga_cursor(0, -1);
    vga_cursor(VGA_ROWS + 1, 0);
    vga_cursor(0, VGA_COLS + 1);

    EXPECT_EQ(0, port_byte_out_fake.call_count);

    // Success
    vga_cursor(10, 17);

    EXPECT_EQ(4, port_byte_out_fake.call_count);

    int index = 10 * 80 + 17;
    EXPECT_EQ(10, vga_cursor_row());
    EXPECT_EQ(17, vga_cursor_col());

    EXPECT_EQ(0x3d4, port_byte_out_fake.arg0_history[0]);
    EXPECT_EQ(14, port_byte_out_fake.arg1_history[0]);

    EXPECT_EQ(0x3d5, port_byte_out_fake.arg0_history[1]);
    EXPECT_EQ(index >> 8, port_byte_out_fake.arg1_history[1]);

    EXPECT_EQ(0x3d4, port_byte_out_fake.arg0_history[2]);
    EXPECT_EQ(15, port_byte_out_fake.arg1_history[2]);

    EXPECT_EQ(0x3d5, port_byte_out_fake.arg0_history[3]);
    EXPECT_EQ(index & 0xff, port_byte_out_fake.arg1_history[3]);
}

TEST_F(VGA, vga_cursor_hide) {
    vga_cursor_hide();

    EXPECT_EQ(2, port_byte_out_fake.call_count);

    EXPECT_EQ(0x3d4, port_byte_out_fake.arg0_history[0]);
    EXPECT_EQ(0xa, port_byte_out_fake.arg1_history[0]);

    EXPECT_EQ(0x3d5, port_byte_out_fake.arg0_history[1]);
    EXPECT_EQ(0x3f, port_byte_out_fake.arg1_history[1]);
}

TEST_F(VGA, vga_cursor_show) {
    port_byte_in_fake.return_val = 0xff;

    vga_cursor_show();

    EXPECT_EQ(2, port_byte_in_fake.call_count);

    EXPECT_EQ(0x3d5, port_byte_in_fake.arg0_history[0]);
    EXPECT_EQ(0x3d5, port_byte_in_fake.arg0_history[1]);

    EXPECT_EQ(4, port_byte_out_fake.call_count);

    EXPECT_EQ(0x3d4, port_byte_out_fake.arg0_history[0]);
    EXPECT_EQ(0xa, port_byte_out_fake.arg1_history[0]);

    EXPECT_EQ(0x3d5, port_byte_out_fake.arg0_history[1]);
    EXPECT_EQ(0xcd, port_byte_out_fake.arg1_history[1]);

    EXPECT_EQ(0x3d4, port_byte_out_fake.arg0_history[2]);
    EXPECT_EQ(0xb, port_byte_out_fake.arg1_history[2]);

    EXPECT_EQ(0x3d5, port_byte_out_fake.arg0_history[3]);
    EXPECT_EQ(0xee, port_byte_out_fake.arg1_history[3]);
}

TEST_F(VGA, vga_cursor_color) {
    vga_color(VGA_FG_RED | VGA_BG_BLUE);

    vga_putc('Q');

    EXPECT_EQ('Q', buff[0]);
    EXPECT_EQ(0x14, buff[1]);
}

TEST_F(VGA, vga_putc) {
    vga_putc('A');
    EXPECT_EQ('A', buff[0]);
    EXPECT_EQ(1, vga_cursor_col());
    EXPECT_EQ(0, vga_cursor_row());

    vga_putc('\b');
    EXPECT_EQ(' ', buff[0]);
    EXPECT_EQ(0, vga_index());

    vga_putc('\b');
    EXPECT_EQ(0, vga_index());

    vga_putc('\n');
    EXPECT_EQ(0, vga_cursor_col());
    EXPECT_EQ(1, vga_cursor_row());

    SetUp();

    vga_cursor(VGA_ROWS - 1, 0);
    ASSERT_EQ(VGA_ROWS - 1, vga_cursor_row());

    vga_putc('\n');
    EXPECT_EQ(VGA_ROWS - 1, vga_cursor_row());
    EXPECT_EQ(0, vga_cursor_col());
}

TEST_F(VGA, vga_puts) {
    vga_puts(0);
    EXPECT_EQ(0, vga_cursor_row());
    EXPECT_EQ(0, vga_cursor_col());

    vga_puts("ab");

    EXPECT_EQ('a', buff[0]);
    EXPECT_EQ(7, buff[1]);

    EXPECT_EQ('b', buff[2]);
    EXPECT_EQ(7, buff[3]);
}

TEST_F(VGA, vga_write) {
    const char data[] = "abc";

    EXPECT_EQ(3, vga_write(data, 3));
    EXPECT_EQ('a', buff[0]);
    EXPECT_EQ('b', buff[2]);
    EXPECT_EQ('c', buff[4]);
}

TEST_F(VGA, vga_puti) {
    vga_puti(10);

    EXPECT_EQ('1', buff[0]);
    EXPECT_EQ(7, buff[1]);

    EXPECT_EQ('0', buff[2]);
    EXPECT_EQ(7, buff[3]);

    vga_clear();
    vga_puti(0);

    EXPECT_EQ('0', buff[0]);
    EXPECT_EQ(7, buff[1]);

    vga_clear();
    vga_puti(-3);

    EXPECT_EQ('-', buff[0]);
    EXPECT_EQ(7, buff[1]);

    EXPECT_EQ('3', buff[2]);
    EXPECT_EQ(7, buff[3]);
}

TEST_F(VGA, vga_putu) {
    vga_putu(10);

    EXPECT_EQ('1', buff[0]);
    EXPECT_EQ(7, buff[1]);

    EXPECT_EQ('0', buff[2]);
    EXPECT_EQ(7, buff[3]);

    vga_clear();
    vga_putu(0);

    EXPECT_EQ('0', buff[0]);
    EXPECT_EQ(7, buff[1]);
}

TEST_F(VGA, vga_putx) {
    vga_putx(168);

    EXPECT_EQ('A', buff[0]);
    EXPECT_EQ(7, buff[1]);

    EXPECT_EQ('8', buff[2]);
    EXPECT_EQ(7, buff[3]);

    vga_clear();
    vga_putx(0);

    EXPECT_EQ('0', buff[0]);
    EXPECT_EQ(7, buff[1]);
}
