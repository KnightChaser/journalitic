#include <systemd/sd-journal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

void format_timestamp(uint64_t usec, char *buffer, size_t buffer_len) {
    time_t t = usec / 1000000;
    struct tm *tm_info = localtime(&t);
    strftime(buffer, buffer_len, "%b %d %H:%M:%S", tm_info);
}

int main(void) {
    sd_journal *journal;
    int r;

    // Open the journal for reading
    r = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (r < 0) {
        perror("Failed to open journal");
        return 1;
    }

    // Seek to the end of the journal to read new entries
    sd_journal_seek_tail(journal);
    sd_journal_next(journal);

    while (true) {
        // Wait for the new journal entries
        sd_journal_wait(journal, (uint64_t)-1);

        // Iterate over the new journal entries
        while (sd_journal_next(journal) > 0) {
            const char *message, *syslog_identifier, *user, *subject;
            uint64_t timestamp;
            int process_id;
            size_t length;

            // Get the timestamp
            r = sd_journal_get_realtime_usec(journal, &timestamp);
            if (r < 0) {
                perror("Failed to get timestamp");
                continue;
            }

            // Get the process name (SYSLOG_IDENTIFIER)
            r = sd_journal_get_data(journal, "SYSLOG_IDENTIFIER", (const void **) &syslog_identifier, &length);
            if (r < 0) {
                syslog_identifier = "unknown";
            } else {
                syslog_identifier += 18; // Skip the "SYSLOG_IDENTIFIER=" prefix
            }

            // Get the process ID
            r = sd_journal_get_data(journal, "_PID", (const void **) &subject, &length);
            if (r < 0) {
                process_id = -1;
            } else {
                process_id = atoi(subject + 5); // Skip the "_PID=" prefix
            }

            // Get the user
            r = sd_journal_get_data(journal, "_HOSTNAME", (const void **) &user, &length);
            if (r < 0) {
                user = "unknown";
            } else {
                user += 10; // Skip the "_HOSTNAME=" prefix
            }

            // Get the message content
            r = sd_journal_get_data(journal, "MESSAGE", (const void **) &message, &length);
            if (r < 0) {
                message = "No message";
            } else {
                message += 8; // Skip the "MESSAGE=" prefix
            }

            // Format the timestamp to match journalctl style
            char time_buffer[64];
            format_timestamp(timestamp, time_buffer, sizeof(time_buffer));

            // Print the log entry in a similar format to journalctl
            printf("%s %s %s[%d]: %s\n", time_buffer, user, syslog_identifier, process_id, message);
        }
    }

    // Close the journal
    sd_journal_close(journal);

    return 0;
}
