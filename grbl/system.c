#include "grbl.h"

void system_init() {
    CONTROL_DDR &= ~CONTROL_MASK;
    ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN));
}

void buttons_check() {
    uint8_t val;
    if ((val = adc_read(CONTROL_JOG_X)) < 63) {
        if (sys.state == STATE_IDLE || sys.state == STATE_JOG) {
            btn_state &= ~(1<<2);
            btn_state |= (1<<0);
        }
    } else if (val < 191) {
        if (sys.state == STATE_IDLE || sys.state == STATE_JOG) {
            btn_state &= ~(1<<0);
            btn_state |= (1<<2);
        }
    } else {
        btn_state &= ~((1<<0)|(1<<2));
    }
    if ((val = adc_read(CONTROL_JOG_Y)) < 63) {
        if (sys.state == STATE_IDLE || sys.state == STATE_JOG) {
            btn_state &= ~(1<<3);
            btn_state |= (1<<1);
        } else if (sys.state == STATE_CYCLE || sys.state == STATE_HOLD) {
            if (!(btn_state & (1<<4))) {
                btn_state |= (1<<4);
                if (sys.f_override > 50) {
                    sys.f_override -= 5;
                    if (sys.f_override < 50) {
                        sys.f_override = 50;
                    }
                    plan_update_velocity_profile_parameters();
                    plan_cycle_reinitialize();
                }
            }
        }
    } else if (val < 191) {
        if (sys.state == STATE_IDLE || sys.state == STATE_JOG) {
            btn_state &= ~(1<<1);
            btn_state |= (1<<3);
        } else if (sys.state == STATE_CYCLE || sys.state == STATE_HOLD) {
            if (!(btn_state & (1<<4))) {
                btn_state |= (1<<4);
                if (sys.f_override < 150) {
                    sys.f_override += 5;
                    if (sys.f_override > 150) {
                        sys.f_override = 150;
                    }
                    plan_update_velocity_profile_parameters();
                    plan_cycle_reinitialize();
                }
            }
        }
    } else {
        btn_state &= ~((1<<1)|(1<<3)|(1<<4));
    }
    if (btn_state) {
        return;
    }
    if (abs(adc_read(CONTROL_OTHER) - (val = adc_read(CONTROL_OTHER))) <= 2) {
        if (val < 21) {
            if (btn) {
                sys_rt_exec_state |= EXEC_RESET;
                report_status_message(STATUS_STOP);
                btn = 0;
            }
        } else if (val < 58) {
            if (btn) {
                if (sys.state == STATE_IDLE) {
                    _delay_ms(30);
                    if (abs(adc_read(CONTROL_OTHER) - val) > 2) {
                        return;
                    }
                    btn_state |= (1<<6);
                } else if (sys.state == STATE_CYCLE) {
                    sys_rt_exec_state |= EXEC_FEED_HOLD;
                } else if (sys.state == STATE_HOLD) {
                    sys_rt_exec_state |= EXEC_CYCLE_START;
                }
                btn = 0;
            }
        } else if (val < 85) {
            if (btn) {
                if (sys.state == STATE_IDLE) {
                    _delay_ms(30);
                    if (abs(adc_read(CONTROL_OTHER) - val) > 2) {
                        return;
                    }
                    if (sys.spindle_speed == SPINDLE_PWM_OFF_VALUE) {
                        spindle_set_speed(SPINDLE_PWM_MIN_VALUE);
                    } else {
                        spindle_set_speed(SPINDLE_PWM_OFF_VALUE);
                    }
                    lcd_state2();
                }
                btn = 0;
            }
        } else if (val < 105) {
            if (btn) {
                if (sys.state == STATE_IDLE) {
                    _delay_ms(30);
                    if (abs(adc_read(CONTROL_OTHER) - val) > 2) {
                        return;
                    }
                    btn_state |= (1<<5);
                }
                btn = 0;
            }
#ifndef COREXY
        } else if (val < 121) {
            if (btn) {
                if (bit_istrue(settings.flags,BITFLAG_HOMING_ENABLE)) {
                    if (sys.state == STATE_IDLE || sys.state == STATE_ALARM) {
                        _delay_ms(30);
                        if (abs(adc_read(CONTROL_OTHER) - val) > 2) {
                            return;
                        }
                        btn_state |= (1<<7);
                    }
                }
                btn = 0;
            }
#endif
        } else {
            btn = 1;
        }
    }
}

uint8_t adc_read(uint8_t pin) {
    ADMUX = ((1<<REFS0)|(1<<ADLAR)) | (pin & 0x07);
    _delay_us(1);
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC)) {}
    return(ADCH);
}

uint8_t system_execute_line(char *line) {
    if (sys.state != STATE_IDLE && sys.state != STATE_ALARM) {
        return(STATUS_IDLE_ERROR);
    }
    uint8_t char_counter = 1;
    float parameter;
    float value;
    switch(line[1]) {
    case '$':
    case 'I':
        if (line[2] != 0) {
            return(STATUS_INVALID_STATEMENT);
        }
        if (line[1] == '$') {
            report_grbl_settings();
        } else {
            sys.state2 = STATE_UART;
            sys.f_override = 100;
            pl_data.feed_rate = 1200;
            pl_data.spindle_speed = 255;
            pl_data.units = 0;
            pl_data.distance = 0;
            pl_data.condition = PL_COND_FLAG_RAPID_MOTION;
            pl_data.gc_pos[X_AXIS] = ((pl_data.xyz[X_AXIS] = plan_get_position(X_AXIS)) - wco[X_AXIS]) / settings.steps_per_mm[X_AXIS];
            pl_data.gc_pos[Y_AXIS] = ((pl_data.xyz[Y_AXIS] = plan_get_position(Y_AXIS)) - wco[Y_AXIS]) / settings.steps_per_mm[Y_AXIS];
            spindle_set_speed(SPINDLE_PWM_OFF_VALUE);
            lcd_state2();
            report_build_info();
        }
        return(STATUS_OK);
    case 'R':
        if (line[2] != 'S' || line[3] != 'T' || line[4] != 0) {
            return(STATUS_INVALID_STATEMENT);
        }
        settings_restore();
        st_generate_step_dir_invert_masks();
        return(STATUS_OK);
    default:
        if (!read_float(line, &char_counter, &value)) {
            return(STATUS_BAD_NUMBER_FORMAT);
        }
        char_counter++;
        if (!read_float(line, &char_counter, &parameter)) {
            return(STATUS_BAD_NUMBER_FORMAT);
        }
        if (line[char_counter] != 0) {
            return(STATUS_INVALID_STATEMENT);
        }
        return(settings_store_global_setting((uint8_t)parameter, value));
    }
    return(STATUS_INVALID_STATEMENT);
}

#ifdef COREXY
int32_t system_convert_corexy_to_x_axis_steps(int32_t *steps) {
    return((steps[A_MOTOR] + steps[B_MOTOR]) / 2);
}

int32_t system_convert_corexy_to_y_axis_steps(int32_t *steps) {
    return((steps[A_MOTOR] - steps[B_MOTOR]) / 2);
}
#endif

void system_set_exec_state_flag(uint8_t mask) {
    uint8_t sreg = SREG;
    cli();
    sys_rt_exec_state |= (mask);
    SREG = sreg;
}
