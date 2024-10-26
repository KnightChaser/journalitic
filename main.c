#include <systemd/sd-journal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

void formatTimestamp(uint64_t usec, char *buffer, size_t bufferLen) {
    time_t t = usec / 1000000;
    struct tm *tmInfo = localtime(&t);
    strftime(buffer, bufferLen, "%b %d %H:%M:%S", tmInfo);
}

const char *getJournalField(sd_journal *journal, const char *fieldName, int prefixLength) {
    const char *fieldData;
    size_t length;
    int r = sd_journal_get_data(journal, fieldName, (const void **) &fieldData, &length);
    if (r < 0) {
        return NULL;
    }
    return fieldData + prefixLength; // Skip the prefix (e.g., "FIELD_NAME=")
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
        sd_journal_wait(journal, (uint64_t) -1);

        // Iterate over the new journal entries
        while (sd_journal_next(journal) > 0) {
            uint64_t timestamp;
            int processId;
            const char *message, *syslogIdentifier, *user;

            // Get the timestamp
            r = sd_journal_get_realtime_usec(journal, &timestamp);
            if (r < 0) {
                perror("Failed to get timestamp");
                continue;
            }

            // Use the generalized function to get the specific fields
            syslogIdentifier = getJournalField(journal, "SYSLOG_IDENTIFIER", 18);
            if (!syslogIdentifier) syslogIdentifier = "unknown";

            const char *pidStr = getJournalField(journal, "_PID", 5);
            processId = pidStr ? atoi(pidStr) : -1;

            user = getJournalField(journal, "_HOSTNAME", 10);
            if (!user) user = "unknown";

            message = getJournalField(journal, "MESSAGE", 8);
            if (!message) message = "No message";

            // Format the timestamp to match journalctl style
            char timeBuffer[64];
            formatTimestamp(timestamp, timeBuffer, sizeof(timeBuffer));

            // Print the log entry in a similar format to journalctl
            printf("%s %s %s[%d]: %s\n", timeBuffer, user, syslogIdentifier, processId, message);
        }
    }

    // Close the journal
    sd_journal_close(journal);

    return 0;
}
