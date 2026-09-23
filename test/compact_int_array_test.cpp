#include <gtest/gtest.h>
#include <misc/compact_int_array.h>

TEST(compact_int_array, Blah1) {

    using arr_type = misc::compact_int_array<20, 5>;
    [[maybe_unused]]arr_type arr;
    arr.storage[0] = 0b0001'0010;
    arr.storage[1] = 0b0011'0100;
    arr.storage[2] = 0b0101'0110;
    arr.storage[3] = 0b1000'0111;

    ASSERT_EQ(arr_type::int_bit_width, 5);
    ASSERT_EQ(arr.get(0), 0b10010);
    ASSERT_EQ(arr.get(1), 0b00000);
    ASSERT_EQ(arr.get(2), 0b01101);
    ASSERT_EQ(arr.get(3), 0b01100);
    ASSERT_EQ(arr.get(4), 0b10101);

    arr.set(0, 0b10011);
    ASSERT_EQ(arr.get(0), 0b10011);
    arr.set(1, 0b00001);
    ASSERT_EQ(arr.get(1), 0b00001);
    arr.set(2, 0b01100);
    ASSERT_EQ(arr.get(2), 0b01100);
    arr.set(3, 0b01101);
    ASSERT_EQ(arr.get(3), 0b01101);
    arr.set(4, 0b10100);

    ASSERT_EQ(arr.get(0), 0b10011);
    ASSERT_EQ(arr.get(1), 0b00001);
    ASSERT_EQ(arr.get(2), 0b01100);
    ASSERT_EQ(arr.get(3), 0b01101);
    ASSERT_EQ(arr.get(4), 0b10100);
}

TEST(compact_int_array, Blah2) {

    using arr_type = misc::compact_int_array<258, 5>;
    [[maybe_unused]]arr_type arr;
    size_t arr_val = 0b100000110'100000101'100000100'100000011'100000010'100000001;
    
    uint8_t* ptr = arr.storage;
    uint8_t* arr_val_ptr = reinterpret_cast<uint8_t*>(&arr_val);
    for (size_t i = 0; i < sizeof(arr.storage); ++i){
        *ptr++ = *arr_val_ptr++;
    }

    ASSERT_EQ(arr_type::int_bit_width, 9);
    ASSERT_EQ(arr.get(0), 0b100000001);
    ASSERT_EQ(arr.get(1), 0b100000010);
    ASSERT_EQ(arr.get(2), 0b100000011);
    ASSERT_EQ(arr.get(3), 0b100000100);
    ASSERT_EQ(arr.get(4), 0b100000101);

    arr.set(0, 0b000000001);
    arr.set(1, 0b000000010);
    arr.set(2, 0b000000011);
    arr.set(3, 0b000000100);
    arr.set(4, 0b000000101);

    ASSERT_EQ(arr.get(0), 0b000000001);
    ASSERT_EQ(arr.get(1), 0b000000010);
    ASSERT_EQ(arr.get(2), 0b000000011);
    ASSERT_EQ(arr.get(3), 0b000000100);
    ASSERT_EQ(arr.get(4), 0b000000101);
}

TEST(compact_int_array, Blah3) {

    using arr_type = misc::compact_int_array<255, 5>;
    [[maybe_unused]]arr_type arr;
    size_t arr_val = 0b10000110'10000101'10000100'10000011'10000010'10000001;
    
    uint8_t* ptr = arr.storage;
    uint8_t* arr_val_ptr = reinterpret_cast<uint8_t*>(&arr_val);
    for (size_t i = 0; i < sizeof(arr.storage); ++i){
        *ptr++ = *arr_val_ptr++;
    }

    ASSERT_EQ(arr_type::int_bit_width, 8);
    ASSERT_EQ(arr.get(0), 0b10000001);
    ASSERT_EQ(arr.get(1), 0b10000010);
    ASSERT_EQ(arr.get(2), 0b10000011);
    ASSERT_EQ(arr.get(3), 0b10000100);
    ASSERT_EQ(arr.get(4), 0b10000101);

    arr.set(0, 0b00000001);
    arr.set(1, 0b00000010);
    arr.set(2, 0b00000011);
    arr.set(3, 0b00000100);
    arr.set(4, 0b00000101);

    ASSERT_EQ(arr.get(0), 0b00000001);
    ASSERT_EQ(arr.get(1), 0b00000010);
    ASSERT_EQ(arr.get(2), 0b00000011);
    ASSERT_EQ(arr.get(3), 0b00000100);
    ASSERT_EQ(arr.get(4), 0b00000101);
}

TEST(compact_int_array, Blah4) {

    using arr_type = misc::compact_int_array<3, 5>;
    [[maybe_unused]]arr_type arr;
    size_t arr_val = 0b01'00'11'10'01;
    
    uint8_t* ptr = arr.storage;
    uint8_t* arr_val_ptr = reinterpret_cast<uint8_t*>(&arr_val);
    for (size_t i = 0; i < sizeof(arr.storage); ++i){
        *ptr++ = *arr_val_ptr++;
    }

    ASSERT_EQ(arr_type::int_bit_width, 2);
    ASSERT_EQ(arr.get(0), 0b01);
    ASSERT_EQ(arr.get(1), 0b10);
    ASSERT_EQ(arr.get(2), 0b11);
    ASSERT_EQ(arr.get(3), 0b00);
    ASSERT_EQ(arr.get(4), 0b01);

    arr.set(0, 0b10);
    arr.set(1, 0b01);
    arr.set(2, 0b00);
    arr.set(3, 0b11);
    arr.set(4, 0b10);

    ASSERT_EQ(arr.get(0), 0b10);
    ASSERT_EQ(arr.get(1), 0b01);
    ASSERT_EQ(arr.get(2), 0b00);
    ASSERT_EQ(arr.get(3), 0b11);
    ASSERT_EQ(arr.get(4), 0b10);
}


TEST(compact_int_vector, Blah1) {

    misc::compact_int_vector<3> v;
    ASSERT_EQ(v.size(), 0);
    ASSERT_EQ(v.capacity(), 0);
    v.push_back(1);
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(v.capacity(), 4);
    v.push_back(2);
    ASSERT_EQ(v.size(), 2);
    v.push_back(3);
    ASSERT_EQ(v.size(), 3);
    v.push_back(1);
    ASSERT_EQ(v.size(), 4);
    ASSERT_EQ(v.capacity(), 4);
    ASSERT_EQ(v.get(0), 1);
    ASSERT_EQ(v.get(1), 2);
    ASSERT_EQ(v.get(2), 3);
    ASSERT_EQ(v.get(3), 1);

    v.push_back(2);
    ASSERT_EQ(v.size(), 5);
    ASSERT_EQ(v.capacity(), 8);
    v.push_back(3);
    ASSERT_EQ(v.size(), 6);
    v.push_back(1);
    ASSERT_EQ(v.size(), 7);
    v.push_back(2);
    ASSERT_EQ(v.size(), 8);
    ASSERT_EQ(v.get(0), 1);
    ASSERT_EQ(v.get(1), 2);
    ASSERT_EQ(v.get(2), 3);
    ASSERT_EQ(v.get(3), 1);
    ASSERT_EQ(v.get(4), 2);
    ASSERT_EQ(v.get(5), 3);
    ASSERT_EQ(v.get(6), 1);
    ASSERT_EQ(v.get(7), 2);
    
}