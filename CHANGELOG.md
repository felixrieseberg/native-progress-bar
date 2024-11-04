# v1.0.2
- Don't update natively if JS values are unchanged (meaning that you can call `.progress = 99`
  as often as you want - we only issue a new UI if the value is actually changed)

# v1.0.1
- Remove `nan` as a dependency, it was unused

# v1.0.0

- Initial release
