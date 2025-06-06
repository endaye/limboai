/**
 * test_play_animation.h
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#ifndef TEST_PLAY_ANIMATION_H
#define TEST_PLAY_ANIMATION_H

#include "limbo_test.h"

#include "modules/limboai/bt/tasks/bt_task.h"
#include "modules/limboai/bt/tasks/scene/bt_play_animation.h"

#include "scene/animation/animation_player.h"
#include "scene/main/window.h"
#include "scene/resources/animation.h"
#include "scene/resources/animation_library.h"

namespace TestPlayAnimation {

TEST_CASE("[SceneTree][LimboAI] BTPlayAnimation") {
	AnimationPlayer *player = memnew(AnimationPlayer);
	SceneTree::get_singleton()->get_root()->add_child(player);
	player->set_callback_mode_process(AnimationMixer::ANIMATION_CALLBACK_MODE_PROCESS_IDLE);

	Ref<AnimationLibrary> anim_lib = memnew(AnimationLibrary);
	Ref<Animation> anim = memnew(Animation);
	anim->set_name("test");
	anim->set_length(0.1);
	anim->set_loop_mode(Animation::LOOP_NONE);
	REQUIRE(anim_lib->add_animation("test", anim) == OK);
	REQUIRE(player->add_animation_library("", anim_lib) == OK);
	REQUIRE(player->has_animation("test"));

	Ref<BTPlayAnimation> pa = memnew(BTPlayAnimation);
	pa->set_animation_name("test");
	Ref<BBNode> player_param = memnew(BBNode);
	pa->set_animation_player(player_param);
	Node *dummy = memnew(Node);
	SceneTree::get_singleton()->get_root()->add_child(dummy);
	Ref<Blackboard> bb = memnew(Blackboard);

	SUBCASE("When AnimationPlayer doesn't exist") {
		player_param->set_saved_value(NodePath("./NotFound"));
		ERR_PRINT_OFF;
		pa->initialize(dummy, bb, dummy);
		CHECK(pa->execute(0.01666) == BTTask::FAILURE);
		ERR_PRINT_ON;
	}
	SUBCASE("When AnimationPlayer exists") {
		player_param->set_saved_value(player->get_path());
		pa->initialize(dummy, bb, dummy);

		SUBCASE("When not waiting to finish") {
			pa->set_await_completion(0.0);
			CHECK(pa->execute(0.01666) == BTTask::SUCCESS);
			CHECK(player->is_playing());
			CHECK(player->get_current_animation() == "test");
		}
		SUBCASE("When exceeding max wait time") {
			pa->set_await_completion(1.0);
			CHECK(pa->execute(0.01666) == BTTask::RUNNING);
			CHECK(player->is_playing());
			CHECK(player->get_current_animation() == "test");
			ERR_PRINT_OFF;
			CHECK(pa->execute(1.0) == BTTask::SUCCESS);
			ERR_PRINT_ON;
		}
		SUBCASE("When animation finishes playing before wait time runs out") {
			pa->set_await_completion(888.0);
			CHECK(pa->execute(0.01666) == BTTask::RUNNING);
			CHECK(player->is_playing());
			CHECK(player->get_current_animation() == "test");

			player->seek(888.0, true);
			player->notification(Node::NOTIFICATION_INTERNAL_PROCESS);
			CHECK_FALSE(player->is_playing());
			CHECK_FALSE(player->get_current_animation() == "test");
			CHECK(pa->execute(0.01666) == BTTask::SUCCESS);
		}
	}

	memdelete(dummy);
	memdelete(player);
}

} //namespace TestPlayAnimation

#endif // TEST_PLAY_ANIMATION_H
