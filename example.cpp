#include <pulse/pulseaudio.h>
#include <string>

#define UNREFERENCED_PARAMETER( P ) static_cast<void>( ( P ) )

enum class PulseAudioIsLastMeaning
{
    Error,
    RealDevice,
    PreviousDeviceWasLastReal,
};

struct
{
    pa_mainloop* mainLoop;
    pa_mainloop_api* api;
    pa_context* context;
} pulseAudioPointers;

// Used for controlling callback functions.
enum class PulseAudioLoopControl
{
        Stop,
        Run,
};

static PulseAudioLoopControl loopControl = PulseAudioLoopControl::Run;

// Hopefully the enums are explanatory enough
// Basically, if isLast == 0, it's a real device
PulseAudioIsLastMeaning getIsLastMeaning( const int isLast ) noexcept
{
    if ( isLast < 0 )
    {
        return PulseAudioIsLastMeaning::Error;
    }

    if ( isLast > 0 )
    {
        return PulseAudioIsLastMeaning::PreviousDeviceWasLastReal;
    }

    return PulseAudioIsLastMeaning::RealDevice;
}

template <class T> void deviceCallback( const T* i, const int isLast )
{
    static_assert(
            std::is_same<pa_source_info, T>::value
            || std::is_same<pa_sink_info, T>::value,
            "Function should only be used with pa_source_info or pa_sink_info." );

    const auto deviceState = getIsLastMeaning( isLast );
    if ( deviceState == PulseAudioIsLastMeaning::PreviousDeviceWasLastReal )
    {
        loopControl = PulseAudioLoopControl::Stop;
        return;
    }
    else if ( deviceState == PulseAudioIsLastMeaning::Error )
    {
        // LOG( ERROR ) << "Error in deviceCallback function.";
        return;
    }

    if constexpr ( std::is_same<pa_source_info, T>::value )
    {
        // Business logic
    }

    else if constexpr ( std::is_same<pa_sink_info, T>::value )
    {
        // Business logic
    }
}

// This is a poor attempt at trying to make the asynchronous API synchronous.
// Callback functions set `loopControl` to `PulseAudioLoopControl::Stop` when they're done.
void customPulseLoop()
{
    while ( loopControl == PulseAudioLoopControl::Run )
    {
        constexpr auto noReturnValue = nullptr;
        constexpr auto blockForEvents = 1;
        pa_mainloop_iterate(
                pulseAudioPointers.mainLoop, blockForEvents, noReturnValue );
    }

    loopControl = PulseAudioLoopControl::Run;
}

std::string getDeviceName( pa_proplist* p )
{
    if ( !p )
    {
        // LOG( ERROR ) << "proplist not valid.";
    }

    constexpr auto deviceDescription = "device.description";
    if ( !pa_proplist_contains( p, deviceDescription ) )
    {
        // LOG( ERROR ) << "proplist does not contain '" << deviceDescription
         //   << "'.";
        return "ERROR";
    }

    std::string s;
    s.assign( pa_proplist_gets( p, deviceDescription ) );

    // LOG( DEBUG ) << "getDeviceName done with: " << s;
    return s;
}

void deviceCallback( const pa_source_info* i, const int isLast )
{
    const auto deviceState = getIsLastMeaning( isLast );
    if ( deviceState == PulseAudioIsLastMeaning::PreviousDeviceWasLastReal )
    {
        loopControl = PulseAudioLoopControl::Stop;
        return;
    }
    else if ( deviceState == PulseAudioIsLastMeaning::Error )
    {
        // Something's wrong.
    }

    // Business logic

}

void setOutputDevicesCallback( pa_context* c,
        const pa_sink_info* i,
        int isLast,
        void* userdata )
{
    // userdata is set in the pa_context_get_sink_info_list call in main.
    // isLast is describe in getIsLastMeaning above
    // Context is the one you registered in pa_context_set_state_callback
    UNREFERENCED_PARAMETER( userdata );
    UNREFERENCED_PARAMETER( c );

    deviceCallback( i, isLast );
}

void stateCallbackFunction( pa_context* c, void* userdata )
{
    UNREFERENCED_PARAMETER( c );
    UNREFERENCED_PARAMETER( userdata );

    switch ( pa_context_get_state( c ) )
    {
        case PA_CONTEXT_TERMINATED:
            // LOG( ERROR ) << "PA_CONTEXT_TERMINATED in stateCallbackFunction";
            //dumpPulseAudioState();
            return;
        case PA_CONTEXT_CONNECTING:
            // LOG( DEBUG ) << "PA_CONTEXT_CONNECTING";
            return;
        case PA_CONTEXT_AUTHORIZING:
            // LOG( DEBUG ) << "PA_CONTEXT_AUTHORIZING";
            return;
        case PA_CONTEXT_SETTING_NAME:
            // LOG( DEBUG ) << "PA_CONTEXT_SETTING_NAME";
            return;
        case PA_CONTEXT_UNCONNECTED:
            // LOG( DEBUG ) << "PA_CONTEXT_UNCONNECTED";
            return;
        case PA_CONTEXT_FAILED:
            // LOG( DEBUG ) << "PA_CONTEXT_FAILED";
            return;

        case PA_CONTEXT_READY:
            // LOG( DEBUG ) << "PA_CONTEXT_READY";
            loopControl = PulseAudioLoopControl::Stop;
            return;
    }
}

int main() {

    pulseAudioPointers.mainLoop = pa_mainloop_new();

    pulseAudioPointers.api = pa_mainloop_get_api( pulseAudioPointers.mainLoop );

    pulseAudioPointers.context
        = pa_context_new( pulseAudioPointers.api, "your-app-name-here" );

    constexpr auto noCustomUserdata = nullptr;
    pa_context_set_state_callback(
            pulseAudioPointers.context, stateCallbackFunction, noCustomUserdata );

    constexpr auto useDefaultServer = nullptr;
    constexpr auto useDefaultSpawnApi = nullptr;
    pa_context_connect( pulseAudioPointers.context,
            useDefaultServer,
            PA_CONTEXT_NOFLAGS,
            useDefaultSpawnApi );

    customPulseLoop();

    // PA is initialized, we can now start calling functions
    
    // Registers the setOutputDevicesCallback function as a callback
    // and spins in customPulseLoop until it is done.
    pa_context_get_sink_info_list( pulseAudioPointers.context,
            setOutputDevicesCallback,
            noCustomUserdata );
    // Run the loop, callbacks are called and they should set loopControl
    customPulseLoop();

}
