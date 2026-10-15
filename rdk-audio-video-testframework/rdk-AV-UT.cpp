/** @file rdk-AV-UT.cpp
 * @brief UT Core process entry point for the audio/video child target.
 */
#include <ut.h>

/** Initializes UT Core, registers the L4 group, runs tests, and shuts down. */
int main(int argc, char** argv)
{
    UT_status_t status = UT_init(argc, argv);

    if (status != UT_STATUS_OK)
    {
        return -1;
    }

    UTCore::UT_add_suite_withGroupID("L4RdkAudioVideoTest", UT_TESTS_L4);

    status = UT_run_tests();

    UT_exit();

    return (status == UT_STATUS_OK) ? 0 : 1;
}
