#include "../fixtures/util.hpp"

#include <mbgl/util/image.hpp>
#include <mbgl/util/io.hpp>

using namespace mbgl;

TEST(Image, PNGRoundTrip) {
    uint8_t rgba[4] = { 128, 0, 0, 255 };
    util::Image image(util::compress_png(1, 1, rgba));
    EXPECT_EQ(128, image.getData()[0]);
    EXPECT_EQ(0, image.getData()[1]);
    EXPECT_EQ(0, image.getData()[2]);
    EXPECT_EQ(255, image.getData()[3]);
}

TEST(Image, PNGReadNoProfile) {
    util::Image image(util::read_file("test/fixtures/image/no_profile.png"));
    EXPECT_EQ(128, image.getData()[0]);
    EXPECT_EQ(0, image.getData()[1]);
    EXPECT_EQ(0, image.getData()[2]);
    EXPECT_EQ(255, image.getData()[3]);
}

TEST(Image, PNGReadNoProfileAlpha) {
    util::Image image(util::read_file("test/fixtures/image/no_profile_alpha.png"));
    EXPECT_EQ(64, image.getData()[0]);
    EXPECT_EQ(0, image.getData()[1]);
    EXPECT_EQ(0, image.getData()[2]);
    EXPECT_EQ(128, image.getData()[3]);
}

TEST(Image, PNGReadProfile) {
    util::Image image(util::read_file("test/fixtures/image/profile.png"));
    EXPECT_EQ(128, image.getData()[0]);
    EXPECT_EQ(0, image.getData()[1]);
    EXPECT_EQ(0, image.getData()[2]);
    EXPECT_EQ(255, image.getData()[3]);
}

TEST(Image, PNGReadProfileAlpha) {
    util::Image image(util::read_file("test/fixtures/image/profile_alpha.png"));
    EXPECT_EQ(64, image.getData()[0]);
    EXPECT_EQ(0, image.getData()[1]);
    EXPECT_EQ(0, image.getData()[2]);
    EXPECT_EQ(128, image.getData()[3]);
}
