#include <systemd/sd-journal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

int main(void) {
    sd_journal *journal;
    int r;

    // Open the journal for reading
    r = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (r < 0) {
        perror("Failed to open journal");
        return 1;
    }

    // Seek to the end of the journal
    sd_journal_seek_tail(journal);
    sd_journal_next(journal);

    while (true) {
        // Wait for the new journal entries
        sd_journal_wait(journal, (uint64_t) -1);

        // Iterate over the new journal entries
        while (sd_journal_next(journal) > 0) {
            const void *data;
            size_t length;

            // Get the data and the length of the journal entry
            r = sd_journal_get_data(journal, "MESSAGE", &data, &length);
            if (r < 0) {
                perror("Failed to get data");
                return 1;
            }

            printf("MESSAGE=%.*s\n", (int) length, (char *) data);
        }
    }

    // Close the journal
    sd_journal_close(journal);

    return 0;
}