// clang-format off
ksNew (16,
    keyNew (PREFIX, KEY_VALUE, "@CONFIG_FILEPATH@", KEY_END),

    keyNew (PREFIX "/local_date_1", KEY_VALUE, "0000-01-01", KEY_END),
    keyNew (PREFIX "/local_date_2", KEY_VALUE, "9999-12-31", KEY_END),
    keyNew (PREFIX "/local_time_1", KEY_VALUE, "00:00:00", KEY_END),
    keyNew (PREFIX "/local_time_2", KEY_VALUE, "23:59:60", KEY_END),
    keyNew (PREFIX "/local_time_3", KEY_VALUE, "23:59:59.99999", KEY_END),
    keyNew (PREFIX "/local_datetime_1", KEY_VALUE, "2000-12-31T10:00:00Z", KEY_END),
    keyNew (PREFIX "/local_datetime_2", KEY_VALUE, "1985-04-12T23:20:50.52Z", KEY_END),
    keyNew (PREFIX "/local_datetime_3", KEY_VALUE, "1990-12-31T23:59:60Z", KEY_END),
    keyNew (PREFIX "/offset_datetime_1", KEY_VALUE, "1600-02-29T23:59:59+23:59", KEY_END),
    keyNew (PREFIX "/offset_datetime_2", KEY_VALUE, "1937-01-01T12:00:27.87+00:20", KEY_END),

    KS_END
)
