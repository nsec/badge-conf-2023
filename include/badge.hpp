/*
 * Copyright 2023 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 */

#ifndef NSEC_RUNTIME_BADGE_HPP
#define NSEC_RUNTIME_BADGE_HPP

namespace nsec::runtime {

/*
 * Current state of the badge.
 */
class badge {
public:
	badge() = default;

	/* Deactivate copy and assignment. */
	badge(const badge&) = delete;
	badge(badge&&) = delete;
	badge& operator=(const badge&) = delete;
	badge& operator=(badge&&) = delete;

	void setup();
};
} // namespace nsec::runtime

#endif // NSEC_RUNTIME_BADGE_HPP
