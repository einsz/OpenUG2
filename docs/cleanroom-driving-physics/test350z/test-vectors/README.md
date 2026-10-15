# Synthetic configuration vectors

`profile.json` is the provisional configuration contract, not a captured runtime record. Base vehicle and tier come from the local preset candidate; selection count comes from independently parsed local packages and static composition. Every field containing `assumption` is unverified against the original opening runtime.

`composition.json` is wholly invented input/output data written by the analyst and reviewer to express the inferred package-selection rules. IDs, package contents and combinations are synthetic. Expected selections follow the functional rules in SPEC.md; no original runtime output is asserted. These vectors cover retaining prerequisite components, exclusions, rejected missing requirements, skipping unavailable levels, and category ordering. They contain no proprietary tables.

Reviewer additions exercise already-selected no-op behavior, stored descriptor order and negative sentinels. Each case includes its synthetic provenance. Host error/switch acceptance cases are specified in SPEC.md; fixtures must use invented records.
