#include <assert.h>

#include "sw_timer.h"

/**
 * @brief Software timer type.
 *
 */
typedef struct SW_TIMER
{
	// A timer relative time
	uint32_t time;

	// A timer period
	uint32_t period;

	// A timer mode of operations
	uint32_t mode;

	// A pointer to the callback function
	sw_timer_func_ptr_t callback;

	// A pointer to the callback argument
	sw_timer_arg_ptr_t arg;

	// A pointer to the next node
	struct SW_TIMER *next;

	// A pointer to the previous node
	struct SW_TIMER *prev;
} sw_timer_t;

/**
 * @brief Software timer private members type.
 *
 */
typedef struct PRIVATE_MEMBERS
{
	sw_timer_t *head;
	set_physical_sw_timer_func_t set_physical_timer;
	get_physical_sw_timer_counter_func_t get_physical_sw_timer_counter;
} sw_timer_private_members_t;

sw_timer_private_members_t private_members = { NULL, NULL, NULL };
sw_timer_private_members_t *this = &private_members;

/**
 * @brief Update relative time.
 *
 * If relative time should be greater or equal to 0x80000000, than need
 * reset relative time for all timers.
 *
 * @param timer The pointer to timer than need be updated.
 *
 * @param delta The time than need add to relative time.
 */
static void sw_timer_update_relative_time(sw_timer_t* timer, uint32_t delta);

/**
 * @brief Insert timer to doubly linked list function.
 *
 * @param timer The pointer to doubly linked list.
 *
 * @param new_timer The pointer to the new node that need insert in linked list.
 */
static void sw_timer_insert(sw_timer_t* timer, sw_timer_t* new_timer);

void sw_timer_register_physical_sw_timer_callbacks(
		set_physical_sw_timer_func_t set_physical_timer,
		get_physical_sw_timer_counter_func_t get_physical_sw_timer_counter)
{
	this->set_physical_timer = set_physical_timer;
	this->get_physical_sw_timer_counter = get_physical_sw_timer_counter;
}

sw_timer_handle_t sw_timer_create(
		uint32_t period,
		sw_timer_mode_t mode,
		sw_timer_func_ptr_t callback,
		sw_timer_arg_ptr_t arg,
		sw_timer_buffer_t *buffer)
{
	assert(sizeof(sw_timer_t) == sizeof(sw_timer_buffer_t));

	sw_timer_t *timer = (sw_timer_t *) buffer;

	timer->time = 0;
	timer->period = period;
	timer->mode = (uint32_t) mode;

	timer->callback = callback;
	timer->arg = arg;

	timer->next = NULL;
	timer->prev = NULL;

	return timer;
}

sw_timer_status_t sw_timer_update(
		sw_timer_handle_t timer,
		uint32_t period,
		sw_timer_mode_t mode,
		sw_timer_func_ptr_t callback,
		sw_timer_arg_ptr_t arg)
{
	sw_timer_status_t status = SW_TIMER_STATUS_OK;

	if ((sw_timer_t *) timer != NULL) {
		if (((sw_timer_t *) timer)->time == 0) {
			((sw_timer_t *) timer)->period = period;
			((sw_timer_t *) timer)->mode = (uint32_t) mode;

			((sw_timer_t *) timer)->callback = callback;
			((sw_timer_t *) timer)->arg = arg;
		} else {
			sw_timer_stop(timer);

			((sw_timer_t *) timer)->period = period;
			((sw_timer_t *) timer)->mode = (uint32_t) mode;

			((sw_timer_t *) timer)->callback = callback;
			((sw_timer_t *) timer)->arg = arg;

			sw_timer_start(timer);
		}
	} else {
		status = SW_TIMER_STATUS_ERROR_TIMER_NOT_EXIST;
	}

	return status;
}

sw_timer_status_t sw_timer_start(sw_timer_handle_t timer)
{
	sw_timer_status_t status = SW_TIMER_STATUS_OK;

	if ((sw_timer_t *) timer != NULL) {
		if (this->head == NULL) {
			this->head = (sw_timer_t *) timer;

			this->head->time = this->head->period;

			/* Start physical timer */
			if (this->set_physical_timer != NULL)
				this->set_physical_timer(this->head->period);
			else
				status = SW_TIMER_STATUS_ERROR_PHYSICAL_TIMER_CALLBACKS_NOT_REGISTERED;
		} else {
			if ((this->set_physical_timer != NULL) && (this->get_physical_sw_timer_counter != NULL)) {
				sw_timer_update_relative_time(
						(sw_timer_t *) timer,
						this->head->time
						- this->get_physical_sw_timer_counter()
						+ ((sw_timer_t *) timer)->period);

				if (((sw_timer_t *) timer)->time < this->head->time) {
					/* Restart physical timer */
					this->set_physical_timer(
							((sw_timer_t *) timer)->time
							- (this->head->time - this->get_physical_sw_timer_counter()));

					((sw_timer_t *) timer)->next = this->head;
					this->head->prev = ((sw_timer_t *) timer);
					this->head = ((sw_timer_t *) timer);
				} else {
					sw_timer_insert(this->head->next, ((sw_timer_t *) timer));
				}
			} else {
				status = SW_TIMER_STATUS_ERROR_PHYSICAL_TIMER_CALLBACKS_NOT_REGISTERED;
			}
		}
	} else {
		status = SW_TIMER_STATUS_ERROR_TIMER_NOT_EXIST;
	}

	return status;
}

sw_timer_status_t sw_timer_stop(sw_timer_handle_t timer)
{
	sw_timer_status_t status = SW_TIMER_STATUS_OK;

	if ((sw_timer_t *) timer != NULL) {
		if (this->head) {
			if (this->head == (sw_timer_t *) timer) {
				if (this->head->next != NULL) {
					/* restart physical timer */
					this->set_physical_timer(
							this->head->next->time
							- (this->head->time - this->get_physical_sw_timer_counter()));

					this->head = this->head->next;
					this->head->prev = NULL;

					((sw_timer_t *) timer)->time = 0;
					((sw_timer_t *) timer)->next = NULL;
				} else {
					this->head->time = 0;
					this->head = NULL;

					/* Stop physical timer */
					this->set_physical_timer(0);
				}
			} else {
				((sw_timer_t *) timer)->time = 0;

				if (((sw_timer_t *) timer)->next != NULL) {
					((sw_timer_t *) timer)->next->prev = ((sw_timer_t *) timer)->prev;
					((sw_timer_t *) timer)->prev->next = ((sw_timer_t *) timer)->next;
					((sw_timer_t *) timer)->prev = NULL;
					((sw_timer_t *) timer)->next = NULL;
				} else {
					((sw_timer_t *) timer)->prev->next = NULL;
					((sw_timer_t *) timer)->prev = NULL;
				}
			}
		}
	} else {
		status = SW_TIMER_STATUS_ERROR_TIMER_NOT_EXIST;
	}

	return status;
}

void sw_timer_interrupt_handler()
{
	uint32_t time = this->head->time;

	while (this->head && (this->head->time == time)) {
		void (*callback)(void* arg) = this->head->callback;
		void * arg = this->head->arg;

		if (this->head->mode == SW_TIMER_MODE_SINGLE_SHOT) {
			this->head->time = 0;

			if (this->head->next != NULL) {
				sw_timer_t* next_timer = this->head->next;

				this->head->next = NULL;
				this->head = next_timer;
				this->head->prev = NULL;
			}
			else {
				this->head = NULL;
			}
		} else if (this->head->mode == SW_TIMER_MODE_REPEATING) {
			sw_timer_update_relative_time(this->head, this->head->period);

			if ((this->head->next != NULL) && (this->head->time > this->head->next->time)) {
				this->head = this->head->next;

				sw_timer_insert(this->head, this->head->prev);

				this->head->prev = NULL;
			}
		} else {
			assert(0);
		}

		if (this->head)
		{
			/* Start physical timer for the next shortest time */
			if (this->head->time != time)
				this->set_physical_timer(this->head->time - time);
		} else {
			/* Stop physical timer */
			this->set_physical_timer(0);
		}

		/* Run callback function if exists with argument */
		if (callback != NULL)
			callback(arg);
	}
}

static void sw_timer_update_relative_time(sw_timer_t* timer, uint32_t delta)
{
	if (((timer->time + delta) && 0x80000000) != 0) {
		sw_timer_t* _timer = this->head;

		uint32_t shift_time = _timer->time - this->get_physical_sw_timer_counter();

		while (_timer) {
			_timer->time -= shift_time;

			_timer = _timer->next;
		}
	}

	timer->time += delta;
}

static void sw_timer_insert(sw_timer_t* timer, sw_timer_t* new_timer)
{
	while (timer) {
		if (new_timer->time < timer->time) {
			new_timer->next = timer;
			new_timer->prev = timer->prev;
			timer->prev->next = new_timer;
			timer->prev = new_timer;

			break;
		}

		if (timer->next == NULL) {
			timer->next = new_timer;
			new_timer->next = NULL;
			new_timer->prev = timer;

			break;
		}

		timer = timer->next;
	}
}
