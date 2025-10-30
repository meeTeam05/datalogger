# ESP32 Firmware - Flow Diagrams

This document describes the operational flows and control logic within the ESP32 firmware.

## System Startup Flow

![System Startup Flow](https://kroki.io/plantuml/svg/eNp1VNtuGjEQffdXTNUXIqVKlltaQDSEsG3acCm7SfsQCZldAxaLTW3DilSV-i39tH5Jx97lmpZ9gD1zPOfMxVxrQ5VZLRIINtqwxShw78uRn8iUvDIztmCw5mNFhSF6zsWSKrqAMY3mUyVXIm7LRCp47Xfsc8DQMxrLlIspTGii2UGERoavudnADwL4uTlN1Sn5Rf82i0kVM5XjXqVaaV863JnM4XK75VcyuLO3Uy6XSlUH3nK6kCJ-IeP7_rt2-YhyqOZXrnzvyoV9KUzAnxl4xd17j2JfWorTxEEtpWS6NVS0zx4OZzyaC6Y1FMlPQgw3CYOGxoQ1722zMW5mnYe882A737gYNxsXltN8Ejm32GzwZicYlIrYF2mAihjuBDfogT9Tw6WAgH1fMRGxxgXfHifETZjUUCg7PJApU9AXVqNOSG2Xg0HvMUB5qmd1IqRhoPh0ZlwlPSnePMoEZdB8YKSiU-YCEyz5K_c5RIrFTNhMmjC0ZhMcZ--sMQ73Ui7rx6rMpFLNsRjD1IRG7MSVS9-lAiXV8ckg7GJBD61h-MKwBUe9h-7IOwfPqxQvL3FpV7GLhd9q8GFw1_euzmG4_V39j-v7zq0jaOuqLYVgkXGWCI6j4FU0hHzB5Mqc2a4jiU-gkHUkI7P4SaTczLgAZL8_A7xUAgobps-cm1qXYvE52U7RYL46YXhroCBkTrqXU0ipEnijzi3ZcLHKJoA2nFzKkwQUM2oDKLW_oZktLI5Pjitry8VSCpyJrpE_v35D90sYwkdcq4QpBwxZQjdIE0bJxCGfgn4PBlTpnHGzMgYd54d0Xn0LjeyTP6HnrWYM_c__6IBbfdxeteYRQzcWPEgfUj3fYXawODUHop47b3fbLg9uCZZuNyxbbhvURi5Pm7m7C0PmbscpO-vVNX7jP-NfUHGk_g==)

## Main Loop Flow

![Main Loop Flow](https://kroki.io/plantuml/svg/eNqFVNuO2jAQffdXTFWpYh8q7t02ROlSINqVgKpKpL4grUxiwCIZI9uA0qr_XtvJhsD2Ej_FM3POmTOTPChNpT7mGSwox-e5EIfnMBNn8kbvWM7gxNeSoiZqz_FAJc1hTZP9VoojphORCQlvw5k9jQy1o6k4c9zChmaKNSI00fzEdQE_CZjnyy3UrB_2wmkZEzJlsrrvDj8MJx13P-U0F5i-Kg3D8NNkcJXSRAiH92H33oVDgTriPxh0e_X7kppex5LTzF2NpRTnqnLQs-dyHe94skemFPTIL0I01xkDXxlAr_sx8NeBNRKskWCN9NvrwG_bcLDCKq0X-DyYGFqOR3FUEBVKsxwWArkW0hpHMYVIU83MWJBuzSRQ-23-gkSIGxvxruhmqGVh-UaESHZgVBMnm2-g9Z2HvET8fAdmtAitcPw0n03vXMpL2lDBgSrFUlAcE7bCjCoNkhnguq5g6lJkH8-Bl0IlmL6QJXpUpzBM-aZUwsw6WJrJ1-VyNokNu8O8kuDATHvrzNBvzAQGqkkN72DxLY4BhQZnAktv1Ljwo3EwM2oim_JPLdOn6G9ybqHEoURqoFgEl-W8XWGyo7hlaa24cqMp0otMPdjJ2bqa2w5uhVpAFC_6vYtkLz5KBIFl22Zioxv9XCU1yY3-K6Zmo_8h22z-xAatpYCywUY3c7G1g9Bmj9mJyQL6HfXKJm_KMlpAt9PJVb2dcN5x8-20qvWXR0Sz-8Y6rqotex-gGNllFwdCHgye-VH9Bnw4ZR8=)

## MQTT Message Processing Flow

![MQTT Message Processing Flow](https://kroki.io/plantuml/svg/eNqNVE1v2kAQvftXTNULHCqKE5o2oSTUwSpS8wXOLZK12INZsd51dxcQrfrfO7t2gvNxCJawPTP73rw3AxfGMm03pYCruyRJr9AYVmB6q1VGj1wWwQe7whJhyxeaSRuYNZcV06yEBcvWhVYbmUdKKA0f44m7WhVmxXK1IxBYMmGwlWGZ5Vtu9_A3APr8eAk1OYrD-LLOKZ2jbuL9wZdB9NnH567xJnwcjeNBHb7krFQyf4UYx_G36PhZSRs4HpzE_ROfjpW0c_4HoR8-vV8zsmCsORM-NNZa7R65Q3cdwsmKZ2tJ5kEY_AsCy61AGBoCPO1_HQ0XI2c0NEbDwWiIhdoNe4vRsOeKRw-yORSOhnyUqIpnnxbMYP50dqY21h1kModIlaW7_6QvQcFhjz8CBYGfcXBK3DPMkG8R2j04zjMq2nGbraATrTBbg-c77wYZMUInZ5YJVRSoe8aWR2Evq-m6XvZprPSO6RysgnlydRSe1WH_nN6PZ0k6RzcIf4aoXJYviappOtlXeN4FWjQJndvJbHpzOY3g5rqG91j3FfWAQEosPsgKNVc5z-A7WL3Bmg9pxRzqASCOa9B3wPgFPeBA54YO6roB6p0vg9dWoKnICo2C7ckQabUSjSHNVMHnoMm1hM98vJH_pPyZYF9Cvtm567XjZHaf63xD3otDXlS3rWoqt0zwvMXzSxWAtLf67B1azd5YLHvevm5rjNIyLg3MJnf3k3nyIN1Qy9_WphpJvMTM4kHmHk2Lv9osBDerNNtojdKmHrvTNO0rIoFMv4KDpWDFGz37udEm0vo2NG7vp4VUutl0Kq933f0wVBUEFxShf8D_0geUZg==)

## STM32 Data Processing Flow

![STM32 Data Processing Flow]()

## Relay State Change Flow

![Relay State Change Flow]()

## Button Press Processing Flow

![Button Press Processing Flow]()

## WiFi State Change Flow

![WiFi State Change Flow]()

## State Update and Publish Flow

![State Update and Publish Flow]()

## MQTT Connection State Machine

![MQTT Connection State Machine]()

## Legend

![Legend]()

**Important Notes:**

- **Green nodes**: Start/end points
- **Yellow nodes**: Decision points
- **Red nodes**: Critical state changes
- **Blue nodes**: Active/stable states

**Flow processing characteristics:**

- WiFi manager auto-retry (5 times with 2s interval)
- MQTT starts only after WiFi is stable for 4s
- All button actions require device (relay) to be ON (except relay toggle)
- Relay state change triggers 500ms delay before sending MQTT status to STM32 (for STM32 boot)
- Exponential backoff for MQTT retry (min 1s, max 60s)
- State synchronization via MQTT retained messages
