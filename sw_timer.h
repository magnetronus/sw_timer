#ifndef SW_TIMER_H
#define SW_TIMER_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief SW_TIMER_TICK_RATE_HZ macro define software timer tick rate
 * and should be defined by application developer.
 *
 * If SW_TIMER_TICK_RATE_HZ macro has not been defined by application
 * developer, the macro will be sets to 1000000, this means that 1 tick
 * of software timer equal to 1 microseconds.
 *
 */
#ifndef SW_TIMER_TICK_RATE_HZ
#define SW_TIMER_TICK_RATE_HZ 1000000
#endif

/**
 * @brief Conversion from seconds to software timer ticks
 */
#define SW_TIMER_CONV_SECONDS_TO_TICKS(val) (val * SW_TIMER_TICK_RATE_HZ)

/**
 * @brief Conversion from milliseconds to software timer ticks
 */
#define SW_TIMER_CONV_MILLISECONDS_TO_TICKS(val) ((val * SW_TIMER_TICK_RATE_HZ) / 1000)

/**
 * @brief Conversion from microseconds to software timer ticks
 */
#define SW_TIMER_CONV_MICROSECONDS_TO_TICKS(val) ((val * SW_TIMER_TICK_RATE_HZ) / 1000000)

/**
 * @brief Timer function pointer type.
 */
typedef void * sw_timer_func_ptr_t;

/**
 * @brief Timer argument pointer type.
 */
typedef void * sw_timer_arg_ptr_t;

/**
 * @brief Timer handle type.
 */
typedef void * sw_timer_handle_t;

/**
 * @brief Function prototype for a set physical timer.
 */
typedef void (*set_physical_sw_timer_func_t)(uint32_t);

/**
 * @brief Function prototype for a get physical timer counter.
 */
typedef uint32_t (*get_physical_sw_timer_counter_func_t)(void);

/**
 * @brief Timer mode type.
 */
typedef enum SW_TIMER_MODE
{
	SW_TIMER_MODE_SINGLE_SHOT,
	SW_TIMER_MODE_REPEATING
} sw_timer_mode_t;

/**
 * @brief Timer status type.
 */
typedef enum SW_TIMER_STATUS
{
	SW_TIMER_STATUS_OK,
	SW_TIMER_STATUS_ERROR_PHYSICAL_TIMER_CALLBACKS_NOT_REGISTERED,
	SW_TIMER_STATUS_ERROR_TIMER_NOT_EXIST
} sw_timer_status_t;

/**
 * @brief Timer buffer type.
 *
 * In line with software engineering best practice, especially when supplying a
 * library that is likely to change in future versions, sw_timer implements a
 * strict data hiding policy. This means the software timer structure used
 * internally by sw_timer is not accessible to application code. However, if
 * the application writer wants to statically allocate the memory required to
 * create a software timer then the size of the queue object needs to be know.
 * The sw_timer_buffer_t structure below is provided for this purpose. Its sizes
 * and alignment requirements are guaranteed to match those of the genuine
 * structure, no matter which architecture is being used. Its contents are
 * somewhat obfuscated in the hope users will recognize that it would be
 * unwise to make direct use of the structure members.
 *
 */
typedef struct SW_TIMER_BUFFER
{
    uint32_t Dummy1;
    uint32_t Dummy2;
    uint32_t Dummy3;
    void *Dummy4;
    void *Dummy5;
    void *Dummy6;
    void *Dummy7;
} sw_timer_buffer_t;

/**
 * @brief Registers physical timer callbacks.
 *
 * @param set_physical_timer Callback handler for set physical timer.
 *
 * @param get_physical_sw_timer_counter Callback handler for get
 * physical timer counter.
 */
void sw_timer_register_physical_sw_timer_callbacks(
		set_physical_sw_timer_func_t set_physical_timer,
		get_physical_sw_timer_counter_func_t get_physical_sw_timer_counter);

/**
 * @brief Creates a new software timer instance, and returns a handle
 * by which the created software timer can be referenced.
 *
 * Application developer must provide the memory buffer sw_timer_buffer_t that will
 * get used by the software timer. Timer is created in the stopped state.
 * The sw_timer_start() API function can be used to transition a timer into
 * the running state.
 *
 * @param period The timer period. The time is defined in tick periods so the
 * macro SW_TIMER_CONV_MILLISECONDS_TO_TICKS can be used to convert a time that
 * has been specified in milliseconds and macro SW_TIMER_CONV_MICROSECONDS_TO_TICKS
 * can be used to convert a time that has been specified in microseconds.
 * For example, if the timer must expire after 100 ticks, then period should be
 * set to 100. Alternatively, if the timer must expire after 500 milliseconds,
 * then period can be set to SW_TIMER_CONV_MILLISECONDS_TO_TICKS(500).
 *
 * @param mode If mode is set to SW_TIMER_MODE_REPEATING then the timer will
 * expire repeatedly with a frequency set by the period parameter. If mode is
 * set to SW_TIMER_MODE_SINGLE_SHOT then the timer will be a one-shot timer and
 * enter the stopped state after it expires.
 *
 * @param callback The function to call when the timer expires.
 *
 * @param arg Argument for the callback function.
 *
 * @param sw_timer_buffer_t Must point to a variable of type sw_timer_t,
 * which is then used to hold the timer’s state.
 *
 * @return The handle to the newly created timer.
 *
 * @note Make sure that the interrupt cannot occur during the execution
 * of this function. Needs disable the interrupt service routine in the
 * main function before using this function.
 *
 * Example usage:
 * @verbatim
 * void callback_func( void )
 * {
 * }
 *
 * void main( void )
 * {
 *	// Return status code for software timer
 *	sw_timer_status_t status;
 *
 *	// Software timer handle
 *	sw_timer_handle_t timer;
 *
 *	// Memory static allocation for software timer
 *	sw_timer_buffer_t sw_timer_buffer;
 *
 *	// Read PRIMASK register, check interrupt status before you disable them
 *	// Returns 0 if they are enabled, or non-zero if disabled
 *	uint32_t prim = __get_PRIMASK();
 *
 *	// Disable interrupts
 *	__disable_irq();
 *
 *	timer = sw_timer_create(SW_TIMER_CONV_MILLISECONDS_TO_TICKS(500), SW_TIMER_MODE_SINGLE_SHOT, &callback_func, NULL, &sw_timer_buffer);
 *
 *	status = sw_timer_start(timer);
 *
 *	status = sw_timer_update(timer, SW_TIMER_CONV_MILLISECONDS_TO_TICKS(100), SW_TIMER_MODE_REPEATING, &callback_func, NULL);
 *
 *	status = sw_timer_stop(timer);
 *
 *	// Enable interrupts back
 *	if (!prim) {
 *		__enable_irq();
 *	}
 * }
 * @endverbatim
 */
sw_timer_handle_t sw_timer_create(
		uint32_t period,
		sw_timer_mode_t mode,
		sw_timer_func_ptr_t callback,
		sw_timer_arg_ptr_t arg,
		sw_timer_buffer_t *buffer);

/**
 * @brief Updates timer's parameters.
 *
 * sw_timer_update() updates a timer parameters for timer that was previously
 * created using the sw_timer_create() API function. If a timer had not been
 * started yet, this API function update timer's parameters; but if the
 * timer had already been started, than this API function first call the
 * sw_timer_stop() API function, than update timer's parameters, and finally
 * call the sw_timer_start() API function.
 *
 * @param timer The handle of the timer that need update it parameters.
 *
 * @param period The timer period. The time is defined in tick periods so the
 * macro SW_TIMER_CONV_MILLISECONDS_TO_TICKS can be used to convert a time that
 * has been specified in milliseconds and macro SW_TIMER_CONV_MICROSECONDS_TO_TICKS
 * can be used to convert a time that has been specified in microseconds.
 * For example, if the timer must expire after 100 ticks, then period should be
 * set to 100. Alternatively, if the timer must expire after 500 milliseconds,
 * then period can be set to SW_TIMER_CONV_MILLISECONDS_TO_TICKS(500).
 *
 * @param mode If mode is set to SW_TIMER_MODE_REPEATING then the timer will
 * expire repeatedly with a frequency set by the period parameter. If mode is
 * set to SW_TIMER_MODE_SINGLE_SHOT then the timer will be a one-shot timer and
 * enter the stopped state after it expires.
 *
 * @param callback The function to call when the timer expires.
 *
 * @param arg Argument for the callback function.
 *
 * @return The timer status code.
 *
 * @note Make sure that the interrupt cannot occur during the execution
 * of this function. Needs disable the interrupt service routine in the
 * main function before using this function.
 *
 * Example usage:
 *
 * See the sw_timer_create() API function example usage scenario.
 */
sw_timer_status_t sw_timer_update(
		sw_timer_handle_t timer,
		uint32_t period,
		sw_timer_mode_t mode,
		sw_timer_func_ptr_t callback,
		sw_timer_arg_ptr_t arg);

/**
 * @brief Starts software timer.
 *
 * @param timer The handle of the timer being started/restarted.
 *
 * sw_timer_start() starts a timer that was previously created using the
 * sw_timer_create() API function. If the timer had already been started,
 * this API function restart the timer.
 *
 * @return The timer status code.
 *
 * @note Make sure that the interrupt cannot occur during the execution
 * of this function. Needs disable the interrupt service routine in the
 * main function before using this function.
 *
 * Example usage:
 *
 * See the sw_timer_create() API function example usage scenario.
 */
sw_timer_status_t sw_timer_start(sw_timer_handle_t timer);

/**
 * @brief Stops software timer.
 *
 * @param timer The handle of the timer being stopped.
 *
 * Stopping a software timer ensures the timer is not in the running state.
 *
 * @return The timer status code.
 *
 * @note Make sure that the interrupt cannot occur during the execution
 * of this function. Needs disable the interrupt service routine in the
 * main function before using this function.
 *
 * Example usage:
 *
 * See the sw_timer_create() API function example usage scenario.
 */
sw_timer_status_t sw_timer_stop(sw_timer_handle_t timer);

/**
 * @brief	Timer interrupt handler.
 * Physical timer interrupt handler should directly call this function.
 */
void sw_timer_interrupt_handler(void);

#endif /* SW_TIMER_H */
