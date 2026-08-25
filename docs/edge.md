Blueprint: Edge TTS Studio
1. Mục tiêu
Edge TTS Studio là ứng dụng desktop Qt6/QML sử dụng framework hiện tại để:
nhập văn bản;
chọn voice;
cấu hình rate/pitch/volume;
gọi Edge TTS thông qua Python plugin;
nhận tiến trình synthesis;
lưu file audio;
phát audio;
quản lý lịch sử;
hiển thị lỗi và trạng thái runtime;
tận dụng Qt Bridge/UI infrastructure có sẵn của framework.
Ứng dụng không tự implement Edge TTS protocol bằng C++.
Edge TTS được coi là một external capability được cung cấp bởi Python plugin.