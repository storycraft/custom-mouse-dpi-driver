# Custom Mouse DPI Driver
하드웨어 DPI 변경이 불가능한 마우스들의 DPI 변경을 가능하게 하는 필터 드라이버

경고: 해당 드라이버는 완전히 테스트 되지 않았습니다

# Install
해당 드라이버는 서명이 되어있지않습니다. 드라이버를 사용하려면 `testsigning`모드를 활성화 해야합니다.

sample_controller와 filter_kmdf 프로젝트를 빌드후 장치 관리자에서 이미 설치된 마우스 드라이버를 빌드된 필터 드라이버로 업데이트 하세요. DPI 수정은 sample_controller.exe로 가능합니다.

## sample_controller 사용법
```
sample_controller.exe
or
sample_controller.exe <원래 dpi> <변경할 x축 dpi> <변경할 y축 dpi>
```

dpi는 0이 아닌 정수여야만 합니다
